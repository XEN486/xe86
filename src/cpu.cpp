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

void CPU::Step() {
	uint8_t opcode = m_Bus->ReadByte(Address20(m_Registers.cs, m_Registers.ip++));
	std::println("{:02x}", opcode);
	m_Functions[opcode]();
}