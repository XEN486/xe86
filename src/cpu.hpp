#ifndef CPU_HPP
#define CPU_HPP

#include "component.hpp"
#include "types.hpp"

#include <functional>
#include <memory>

namespace xe86 {
	enum class Flags : uint16_t {
		OF = 0b0000100000000000,
		DF = 0b0000010000000000,
		IF = 0b0000001000000000,
		TF = 0b0000000100000000,
		SF = 0b0000000010000000,
		ZF = 0b0000000001000000,
		AF = 0b0000000000010000,
		PF = 0b0000000000000100,
		CF = 0b0000000000000001,
	};

	struct Registers {
		// data group
		union { Register16 ax; struct { Register8 al; Register8 ah; }; }; // AX - accumulator
		union { Register16 bx; struct { Register8 bl; Register8 bh; }; }; // BX - base
		union { Register16 cx; struct { Register8 cl; Register8 ch; }; }; // CX - count
		union { Register16 dx; struct { Register8 dl; Register8 dh; }; }; // DX - data

		// pointer and index group
		Register16 sp; // SP - stack pointer
		Register16 bp; // BP - base pointer
		Register16 si; // SI - source index
		Register16 di; // DI - destination index

		// segment registers
		SegmentRegister cs; // CS - code segment
		SegmentRegister ds; // DS - data segment
		SegmentRegister ss; // SS - stack segment
		SegmentRegister es; // ES - extra segment

		// other
		Register16 ip; // IP - instruction pointer
		Flags flags; // FLAGS - flags
	};

	enum class ModRMType {
		Address,
		Register8,
		Register16,
	};

	enum class RegType {
		Register8,
		Register16,
	};

	enum class RegEncoding {
		Register8,
		Register16,
		Segment,
	};

	struct ModRMPart {
		ModRMType type;

		// only one of these will be used at a time
		union {
			uint16_t* reg16;
			uint8_t* reg8;
			uint16_t addr;
		};
		
		uint16_t Read16(std::shared_ptr<Bus> bus, uint16_t segment) {
			switch (type) {
				case ModRMType::Address: return bus->ReadWord((segment * 0x10) + addr);
				case ModRMType::Register16: return *reg16;
				case ModRMType::Register8: {
					std::println(stderr, "reading 8-bit register as 16-bit!!");
					return *reg8;
				}
			}
		}

		uint8_t Read8(std::shared_ptr<Bus> bus, uint16_t segment) {
			switch (type) {
				case ModRMType::Address: return bus->ReadByte((segment * 0x10) + addr);
				case ModRMType::Register8: return *reg8;
				case ModRMType::Register16: {
					std::println(stderr, "reading 16-bit register as 8-bit!!");
					return *reg16 & 0xff;
				}
			}
		}

		void Write16(std::shared_ptr<Bus> bus, uint16_t segment, uint16_t word) {
			switch (type) {
				case ModRMType::Address: bus->WriteWord((segment * 0x10) + addr, word); break;
				case ModRMType::Register16: *reg16 = word; break;
				case ModRMType::Register8: {
					std::println(stderr, "writing 16-bit value to 8-bit register!!");
					*reg8 = word & 0xff;
					break;
				}
			}
		}

		void Write8(std::shared_ptr<Bus> bus, uint16_t segment, uint8_t byte) {
			switch (type) {
				case ModRMType::Address: bus->WriteByte((segment * 0x10) + addr, byte); break;
				case ModRMType::Register8: *reg8 = byte; break;
				case ModRMType::Register16: {
					std::println(stderr, "writing 8-bit value to 16-bit register!!");
					*reg16 = byte;
					break;
				}
			}
		}
	};

	struct RegPart {
		RegType type;

		// only one of these will be used at a time
		union {
			uint16_t* reg16;
			uint8_t* reg8;
		};

		uint16_t Read16() {
			switch (type) {
				case RegType::Register16: return *reg16;
				case RegType::Register8: {
					std::println(stderr, "reading 8-bit register as 16-bit!!");
					return *reg8;
				}
			}
		}

		uint8_t Read8() {
			switch (type) {
				case RegType::Register8: return *reg8;
				case RegType::Register16: {
					std::println(stderr, "reading 16-bit register as 8-bit!!");
					return *reg16 & 0xff;
				}
			}
		}

		void Write16(uint16_t word) {
			switch (type) {
				case RegType::Register16: *reg16 = word; break;
				case RegType::Register8: {
					std::println(stderr, "writing 16-bit value to 8-bit register!!");
					*reg8 = word & 0xff;
					break;
				}
			}
		}

		void Write8(uint8_t byte) {
			switch (type) {
				case RegType::Register8: *reg8 = byte; break;
				case RegType::Register16: {
					std::println(stderr, "writing 8-bit value to 16-bit register!!");
					*reg16 = byte;
					break;
				}
			}
		}
	};

	struct ModRM {
		ModRMPart modrm;
		RegPart reg;
	};

	class CPU : public Component {
	public:
		CPU(std::shared_ptr<Bus> bus) : Component(bus, "CPU") {
			// fill m_Functions with invalid opcodes
			m_Functions.resize(256);
			std::fill(m_Functions.begin(), m_Functions.end(), [this]() { InvalidOpcode(); });

			// now set the proper opcodes
			SetOpcodes();
		}

		void Reset() override {
			// CS:IP = FFFF:0000 on 8086
			m_Registers.cs = 0xffff;
			m_Registers.ip = 0x0000;
		}

		void Step() override;

	private:
		void InvalidOpcode();
		void SetOpcodes();

		ModRM FetchModRM(bool w, RegEncoding encoding);

		uint8_t Fetch8() {
			return m_Bus->ReadByte(Address20(m_Registers.cs, m_Registers.ip++));
		}

		uint16_t Fetch16() {
			uint8_t lo = Fetch8();
			uint8_t hi = Fetch8();
			return (hi << 8) | lo;
		}

		void ClearFlag(Flags flag) {
			// maybe i shouldnt use enum class
			m_Registers.flags = static_cast<Flags>(static_cast<uint16_t>(m_Registers.flags) & ~static_cast<uint16_t>(flag));
		}

		void SetFlag(Flags flag) {
			m_Registers.flags = static_cast<Flags>(static_cast<uint16_t>(m_Registers.flags) | static_cast<uint16_t>(flag));
		}

		void Dump() {
			std::println(
				"ax = {:04x} bx = {:04x} cx = {:04x} dx = {:04x}\n"
				"sp = {:04x} bp = {:04x} si = {:04x} di = {:04x}\n"
				"cs = {:04x} ds = {:04x} ss = {:04x} es = {:04x}\n"
				"ip = {:04x} flags = {:016b}\n"
				"cs:ip = {:04x}:{:04x} ({:05x})",

				static_cast<uint16_t>(m_Registers.ax), static_cast<uint16_t>(m_Registers.bx),
				static_cast<uint16_t>(m_Registers.cx), static_cast<uint16_t>(m_Registers.dx),
				static_cast<uint16_t>(m_Registers.sp), static_cast<uint16_t>(m_Registers.bp),
				static_cast<uint16_t>(m_Registers.si), static_cast<uint16_t>(m_Registers.di),
				static_cast<uint16_t>(m_Registers.cs), static_cast<uint16_t>(m_Registers.ds),
				static_cast<uint16_t>(m_Registers.ss), static_cast<uint16_t>(m_Registers.es),
				static_cast<uint16_t>(m_Registers.ip), static_cast<uint16_t>(m_Registers.flags),
				static_cast<uint16_t>(m_Registers.cs), static_cast<uint16_t>(m_Registers.ip),
				(m_Registers.cs * 0x10) + m_Registers.ip
			);
		}
		
	private:
		Registers m_Registers;
		std::vector<std::function<void()>> m_Functions;
	};
}

#endif