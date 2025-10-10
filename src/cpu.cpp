#include "cpu.hpp"
#include <print>

using namespace xe86;

void CPU::InvalidOpcode() {
	std::println(stderr, "invalid opcode @ {:04x}:{:04x} ({:05x})",
		static_cast<uint16_t>(m_Registers.cs),
		static_cast<uint16_t>(m_Registers.ip - 1),
		static_cast<uint32_t>(Address20(m_Registers.cs, m_Registers.ip - 1))
	);

	exit(1);
}

void CPU::SetOpcodes() {
	// EA - JMP Ap
	m_Functions[0xea] = [this]() {
		// if we update CS and IP directly then it will read from the wrong address
		uint16_t new_ip = Fetch16();
		uint16_t new_cs = Fetch16();

		m_Registers.ip = new_ip;
		m_Registers.cs = new_cs;
	};

	// FA - CLI
	m_Functions[0xfa] = [this]() {
		ClearFlag(Flags::IF); // interrupt flag
	};

	// FC - CLD
	m_Functions[0xfc] = [this]() {
		ClearFlag(Flags::DF); // direction flag
	};
}

void CPU::Step() {
	uint8_t opcode = Fetch8();
	std::println("{:02x}", opcode);
	m_Functions[opcode]();
}