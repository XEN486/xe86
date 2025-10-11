#ifndef BUS_HPP
#define BUS_HPP

#include "types.hpp"
#include <vector>
#include <string>
#include <print>
#include <memory>
#include <functional>

namespace xe86 {
	class MemoryArea {
	public:
		MemoryArea(Address20 start, Address20 end, bool readable, bool writable)
			: m_Start(start), m_End(end), m_Length(end - start + 1), m_Area(m_Length, 0), 
			  m_Readable(readable), m_Writable(writable) {}

		std::vector<uint8_t>& GetArea() { return m_Area; }

		Address20 GetStartAddress() { return m_Start; }
		Address20 GetEndAddress() { return m_End; }
		size_t GetLength() { return m_Length; }

		bool IsReadable() { return m_Readable; }
		bool IsWritable() { return m_Writable; }

		void LoadFromFile(std::string_view filename);

		uint8_t ReadByte(Address20 offset) {
			if (!m_Readable) {
				std::println(stderr, "attempted to read from unreadable memory area [{:05x} -> {:05x}] @ off. {:05x}",
					static_cast<uint32_t>(GetStartAddress()),
					static_cast<uint32_t>(GetEndAddress()),
					static_cast<uint32_t>(offset)
				);

				return 0;
			}

			return m_Area[offset];
		}

		void WriteByte(Address20 offset, uint8_t byte) {
			if (!m_Writable) {
				std::println(stderr, "attempted to write {:02x} to unwritable memory area [{:05x} -> {:05x}] @ off. {:05x}",
					static_cast<uint8_t>(byte),
					static_cast<uint32_t>(GetStartAddress()),
					static_cast<uint32_t>(GetEndAddress()),
					static_cast<uint32_t>(offset)
				);

				return;
			}
			
			m_Area[offset] = byte;
		}

	private:
		Address20 m_Start;
		Address20 m_End;
		size_t m_Length;

		std::vector<uint8_t> m_Area;

		bool m_Readable;
		bool m_Writable;
	};

	struct PortRegistration {
		std::function<void(uint8_t)> write;
		std::function<uint8_t()> read;
		PortAddress16 port;
	};

	/*
	SYSTEM MEMORY MAP
		FFFFF - [TOP OF ADDRESS SPACE]			__
		FE000 - Start of GLaBIOS ROM			  | -- BIOS
		F6000 - Start of Base System ROM area	__|
		F0000 - Reserved						__| -- RESERVED
		C0000 - Start of Expansion Memory area	__| -- ROM
		A0000 - Start of "128KB" area			__| -- GRAPHICS
		00000 - Start of RAM area				__| -- RAM
	*/
	class Bus {
	public:
		Bus(std::string_view bios_rom) {
			AttachMemoryArea(std::make_shared<MemoryArea>(0xfe000, 0xfffff, true, false));	// GLaBIOS ROM
			AttachMemoryArea(std::make_shared<MemoryArea>(0x00000, 0x9ffff, true, true));	// RAM
			m_Memory[0]->LoadFromFile(bios_rom);
		}

		uint8_t ReadByte(Address20 address);
		void WriteByte(Address20 address, uint8_t byte);

		uint16_t ReadWord(Address20 address) {
			return (ReadByte(address + 1) << 8) | ReadByte(address);
		}

		void WriteWord(Address20 address, uint16_t word) {
			WriteByte(address + 0, (word >> 0) & 0xff);
			WriteByte(address + 1, (word >> 8) & 0xff);
		}

		uint8_t ReadByteFromPort(PortAddress16 port) {
			for (auto& p : m_Ports) {
				if (p.port == port) {
					return p.read();
				}
			}

			std::println(stderr, "emulator: reading from unknown port {:02x}", port);
			return 0;
		}

		void WriteByteToPort(PortAddress16 port, uint8_t byte) {
			for (auto& p : m_Ports) {
				if (p.port == port) {
					return p.write(byte);
				}
			}

			std::println(stderr, "emulator: writing to unknown port {:02x} -> {:02x}", byte, port);
		}

		void AttachPort(PortRegistration&& port) {
			for (auto& p : m_Ports) {
				if (p.port == port.port) {
					std::println(stderr, "emulator: trying to reregister port {:02x}", port.port);
					return;
				}
			}

			m_Ports.push_back(std::move(port));
		}

		void AttachMemoryArea(std::shared_ptr<MemoryArea> area) {
			m_Memory.push_back(area);
		}

	private:
		std::vector<std::shared_ptr<MemoryArea>> m_Memory;
		std::vector<PortRegistration> m_Ports;
		std::shared_ptr<MemoryArea> FindArea(Address20 address);
	};
}

#endif