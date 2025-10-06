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

	class CPU : public Component {
	public:
		CPU(std::shared_ptr<Bus> bus) : Component(bus, "CPU") {
			m_Functions.resize(256);
			std::fill(m_Functions.begin(), m_Functions.end(), [=]() { InvalidOpcode(); });
		}

		void Reset() override {
			// CS:IP = FFFF:0000 on 8086
			m_Registers.cs = 0xffff;
			m_Registers.ip = 0x0000;
		}

		void Step() override;

	private:
		void InvalidOpcode();
		
	private:
		Registers m_Registers;
		std::vector<std::function<void()>> m_Functions;
	};
}

#endif