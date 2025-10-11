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

// https://github.com/640-KB/GLaBIOS/blob/26d66b91d807431eff995d5e30330cb48398eec1/src/GLABIOS.ASM#L3220
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

	// B8 - MOV AX, Iv
	m_Functions[0xb8] = [this]() {
		m_Registers.ax = Fetch16();
	};

	// 8B - MOV Gv, Ev
	m_Functions[0x8b] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus));
	};

	// 8E - MOV Sw, Ew
	m_Functions[0x8e] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Segment);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus));
	};

	// 8C - MOV Ew, Sw
	m_Functions[0x8c] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Segment);
		modrm.modrm.Write16(m_Bus, modrm.reg.Read16());
	};
}

ModRM CPU::FetchModRM(bool w, RegEncoding encoding) {
	uint8_t byte = Fetch8();
	ModRM result;

	uint8_t mod	= (byte & 0b11000000) >> 6;
	uint8_t reg	= (byte & 0b00111000) >> 3;
	uint8_t rm	= (byte & 0b00000111) >> 0;
	
	// REG (16-bit)
	if (encoding == RegEncoding::Register16) {
		result.reg.type = RegType::Register16;
		switch (reg) {
			case 0b000: result.reg.reg16 = &m_Registers.ax; break;
			case 0b001: result.reg.reg16 = &m_Registers.cx; break;
			case 0b010: result.reg.reg16 = &m_Registers.dx; break;
			case 0b011: result.reg.reg16 = &m_Registers.bx; break;
			case 0b100: result.reg.reg16 = &m_Registers.sp; break;
			case 0b101: result.reg.reg16 = &m_Registers.bp; break;
			case 0b110: result.reg.reg16 = &m_Registers.si; break;
			case 0b111: result.reg.reg16 = &m_Registers.di; break;
		}
	}

	// REG (8-bit)
	else if (encoding == RegEncoding::Register8) {
		result.reg.type = RegType::Register8;
		switch (reg) {
			case 0b000: result.reg.reg8 = &m_Registers.al; break;
			case 0b001: result.reg.reg8 = &m_Registers.cl; break;
			case 0b010: result.reg.reg8 = &m_Registers.dl; break;
			case 0b011: result.reg.reg8 = &m_Registers.bl; break;
			case 0b100: result.reg.reg8 = &m_Registers.ah; break;
			case 0b101: result.reg.reg8 = &m_Registers.ch; break;
			case 0b110: result.reg.reg8 = &m_Registers.dh; break;
			case 0b111: result.reg.reg8 = &m_Registers.bh; break;
		}
	}

	// REG (segment)
	else if (encoding == RegEncoding::Segment) {
		result.reg.type = RegType::Register16;
		switch (reg) {
			case 0b000: result.reg.reg16 = &m_Registers.es;
			case 0b001: result.reg.reg16 = &m_Registers.cs;
			case 0b010: result.reg.reg16 = &m_Registers.ss;
			case 0b011: result.reg.reg16 = &m_Registers.ds;
		}
	}

	// MOD = 00
	if (mod == 0b00) {
		result.modrm.type = ModRMType::Address;
		switch (rm) {
			case 0b000: result.modrm.addr = m_Registers.bx + m_Registers.si; break;
			case 0b001: result.modrm.addr = m_Registers.bx + m_Registers.di; break;
			case 0b010: result.modrm.addr = m_Registers.bp + m_Registers.si; break;
			case 0b011: result.modrm.addr = m_Registers.bp + m_Registers.di; break;
			case 0b100: result.modrm.addr = m_Registers.si; break;
			case 0b101: result.modrm.addr = m_Registers.di; break;
			case 0b110: result.modrm.addr = Fetch16(); break;
			case 0b111: result.modrm.addr = m_Registers.bx; break;
		}

		return result;
	}

	// MOD = 01
	else if (mod == 0b01) {
		result.modrm.type = ModRMType::Address;
		switch (rm) {
			case 0b000: result.modrm.addr = m_Registers.bx + m_Registers.si + Fetch8(); break;
			case 0b001: result.modrm.addr = m_Registers.bx + m_Registers.di + Fetch8(); break;
			case 0b010: result.modrm.addr = m_Registers.bp + m_Registers.si + Fetch8(); break;
			case 0b011: result.modrm.addr = m_Registers.bp + m_Registers.di + Fetch8(); break;
			case 0b100: result.modrm.addr = m_Registers.si + Fetch8(); break;
			case 0b101: result.modrm.addr = m_Registers.di + Fetch8(); break;
			case 0b110: result.modrm.addr = m_Registers.bp + Fetch8(); break;
			case 0b111: result.modrm.addr = m_Registers.bx + Fetch8(); break;
		}

		return result;
	}

	// MOD = 10
	else if (mod == 0b10) {
		result.modrm.type = ModRMType::Address;
		switch (rm) {
			case 0b000: result.modrm.addr = m_Registers.bx + m_Registers.si + Fetch16(); break;
			case 0b001: result.modrm.addr = m_Registers.bx + m_Registers.di + Fetch16(); break;
			case 0b010: result.modrm.addr = m_Registers.bp + m_Registers.si + Fetch16(); break;
			case 0b011: result.modrm.addr = m_Registers.bp + m_Registers.di + Fetch16(); break;
			case 0b100: result.modrm.addr = m_Registers.si + Fetch16(); break;
			case 0b101: result.modrm.addr = m_Registers.di + Fetch16(); break;
			case 0b110: result.modrm.addr = m_Registers.bp + Fetch16(); break;
			case 0b111: result.modrm.addr = m_Registers.bx + Fetch16(); break;
		}

		return result;
	}

	// MOD = 11 (16-bit)
	if (w) {
		result.modrm.type = ModRMType::Register16;
		switch (rm) {
			case 0b000: result.modrm.reg16 = &m_Registers.ax; break;
			case 0b001: result.modrm.reg16 = &m_Registers.cx; break;
			case 0b010: result.modrm.reg16 = &m_Registers.dx; break;
			case 0b011: result.modrm.reg16 = &m_Registers.bx; break;
			case 0b100: result.modrm.reg16 = &m_Registers.sp; break;
			case 0b101: result.modrm.reg16 = &m_Registers.bp; break;
			case 0b110: result.modrm.reg16 = &m_Registers.si; break;
			case 0b111: result.modrm.reg16 = &m_Registers.di; break;
		}
	}

	// MOD = 11 (8-bit)
	else {
		result.modrm.type = ModRMType::Register8;
		switch (rm) {
			case 0b000: result.modrm.reg8 = &m_Registers.al; break;
			case 0b001: result.modrm.reg8 = &m_Registers.cl; break;
			case 0b010: result.modrm.reg8 = &m_Registers.dl; break;
			case 0b011: result.modrm.reg8 = &m_Registers.bl; break;
			case 0b100: result.modrm.reg8 = &m_Registers.ah; break;
			case 0b101: result.modrm.reg8 = &m_Registers.ch; break;
			case 0b110: result.modrm.reg8 = &m_Registers.dh; break;
			case 0b111: result.modrm.reg8 = &m_Registers.bh; break;
		}
	}

	return result;
}

void CPU::Step() {
	uint8_t opcode = Fetch8();
	std::println("{:02x}", opcode);
	m_Functions[opcode]();
}