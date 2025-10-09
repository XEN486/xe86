#ifndef BUS_HPP
#define BUS_HPP

#include "types.hpp"
#include <vector>
#include <string>
#include <print>

namespace xe86 {
	class MemoryArea {
	public:
		MemoryArea(Address20 start, Address20 end, bool readable, bool writable)
			: m_Start(start), m_End(end), m_Length(end - start + 1), m_Area(m_Length, 0), m_Readable(readable), m_Writable(writable) {}

		std::vector<uint8_t>& GetArea() { return m_Area; }

		Address20 GetStartAddress() { return m_Start; }
		Address20 GetEndAddress() { return m_End; }
		size_t GetLength() { return m_Length; }

		bool IsReadable() { return m_Readable; }
		bool IsWritable() { return m_Writable; }

		void LoadFromFile(std::string_view filename);
		void LoadFromFile(std::string_view filename, size_t offset);

	private:
		Address20 m_Start;
		Address20 m_End;
		size_t m_Length;

		std::vector<uint8_t> m_Area;

		bool m_Readable;
		bool m_Writable;
	};

	/*
	SYSTEM MEMORY MAP
		FFFFF - [TOP OF ADDRESS SPACE]			__
		FE000 - Start of GLaBIOS ROM			  | -- BIOS
		F6000 - Start of Base System ROM area	__|
		F0000 - Reserved						  | -- ROM
		C0000 - Start of Expansion Memory area	__|
		A0000 - Start of "128KB" area			__| -- GRAPHICS
		00000 - Start of RAM area				__| -- RAM
	*/
	class Bus {
	public:
		Bus(std::string_view bios_rom) {
			m_Memory.emplace_back(0xf6000, 0xfffff, true, false);	// BIOS ROM
			m_Memory.emplace_back(0xc0000, 0xeffff, true, false);	// Expansion ROM
			m_Memory.emplace_back(0xa0000, 0xbffff, true, true);	// Graphics
			m_Memory.emplace_back(0x00000, 0x9ffff, true, true);	// RAM
			m_Memory[0].LoadFromFile(bios_rom, 0x8000);
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

	private:
		std::vector<MemoryArea> m_Memory;
		MemoryArea* FindArea(Address20 address);
	};
}

#endif