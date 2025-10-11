#include "cpu.hpp"
#include <print>

using namespace xe86;

void CPU::InvalidOpcode() {
	std::println(stderr, "invalid opcode @ {:04x}:{:04x} ({:05x})",
		static_cast<uint16_t>(m_Registers.cs),
		static_cast<uint16_t>(m_Registers.ip - 1),
		static_cast<uint32_t>(Address20(m_Registers.cs, m_Registers.ip - 1))
	);

	Dump();
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

	// 88 - MOV Eb, Gb
	m_Functions[0x88] = [this]() {
		ModRM modrm = FetchModRM(false, RegEncoding::Register8);
		modrm.modrm.Write8(m_Bus, m_Registers.ds, modrm.reg.Read8());
	};

	// 89 - MOV Ev, Gv
	m_Functions[0x89] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, modrm.reg.Read16());
	};

	// 8A - MOV Gb, Eb
	m_Functions[0x8a] = [this]() {
		ModRM modrm = FetchModRM(false, RegEncoding::Register8);
		modrm.reg.Write8(modrm.modrm.Read8(m_Bus, m_Registers.ds));
	};

	// 8B - MOV Gv, Ev
	m_Functions[0x8b] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus, m_Registers.ds)); // TODO: the segment can be changed using a segment prefix
	};

	// 8C - MOV Ew, Sw
	m_Functions[0x8c] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Segment);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, modrm.reg.Read16());
	};

	// 8E - MOV Sw, Ew
	m_Functions[0x8e] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Segment);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus, m_Registers.ds));
	};

	// 8C - MOV Ew, Sw
	m_Functions[0x8c] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Segment);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, modrm.reg.Read16());
	};

	// A0 - MOV AL, Ob
	m_Functions[0xa0] = [this]() {
		m_Registers.al = m_Bus->ReadByte((m_Registers.ds * 0x10) + Fetch16());
	};

	// A1 - MOV AX, Ov
	m_Functions[0xa1] = [this]() {
		m_Registers.ax = m_Bus->ReadWord((m_Registers.ds * 0x10) + Fetch16());
	};

	// A2 - MOV Ob, AL
	m_Functions[0xa2] = [this]() {
		uint16_t addr = (m_Registers.ds * 0x10) + Fetch16();
		m_Bus->WriteByte(addr, m_Registers.al);
	};

	// A3 - MOV Ov, AX
	m_Functions[0xa3] = [this]() {
		uint16_t addr = (m_Registers.ds * 0x10) + Fetch16();
		m_Bus->WriteWord(addr, m_Registers.ax);
	};

	// A4 - MOVSB
	m_Functions[0xa4] = [this]() {
		m_Bus->WriteByte((m_Registers.es * 0x10) + m_Registers.di, m_Bus->ReadByte((m_Registers.ds * 0x10) + m_Registers.si));

		if (GetFlag(Flags::DF)) {
			m_Registers.si--;
			m_Registers.di--;
		} else {
			m_Registers.si++;
			m_Registers.di++;
		}
	};

	// A5 - MOVSW
	m_Functions[0xa5] = [this]() {
		m_Bus->WriteWord((m_Registers.es * 0x10) + m_Registers.di, m_Bus->ReadWord((m_Registers.ds * 0x10) + m_Registers.si));

		if (GetFlag(Flags::DF)) {
			m_Registers.si -= 2;
			m_Registers.di -= 2;
		} else {
			m_Registers.si += 2;
			m_Registers.di += 2;
		}
	};

	// B0 - MOV AL, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.al = Fetch8();
	};

	// B1 - MOV CL, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.cl = Fetch8();
	};

	// B2 - MOV DL, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.dl = Fetch8();
	};

	// B3 - MOV BL, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.bl = Fetch8();
	};

	// B4 - MOV AH, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.ah = Fetch8();
	};

	// B5 - MOV CH, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.ch = Fetch8();
	};

	// B6 - MOV DH, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.dh = Fetch8();
	};

	// B7 - MOV BH, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.bh = Fetch8();
	};

	// B8 - MOV AX, Iv
	m_Functions[0xb8] = [this]() {
		m_Registers.ax = Fetch16();
	};

	// B9 - MOV CX, Iv
	m_Functions[0xb9] = [this]() {
		m_Registers.cx = Fetch16();
	};

	// BA - MOV DX, Iv
	m_Functions[0xba] = [this]() {
		m_Registers.dx = Fetch16();
	};

	// BB - MOV BX, Iv
	m_Functions[0xbb] = [this]() {
		m_Registers.bx = Fetch16();
	};

	// BC - MOV SP, Iv
	m_Functions[0xbc] = [this]() {
		m_Registers.sp = Fetch16();
	};

	// BD - MOV BP, Iv
	m_Functions[0xbd] = [this]() {
		m_Registers.bp = Fetch16();
	};

	// BE - MOV SI, Iv
	m_Functions[0xbe] = [this]() {
		m_Registers.si = Fetch16();
	};

	// BF - MOV DI, Iv
	m_Functions[0xbf] = [this]() {
		m_Registers.di = Fetch16();
	};

	// C6 - MOV Eb, Ib
	m_Functions[0xc6] = [this]() {
		ModRM modrm = FetchModRM(false, RegEncoding::Register8);
		modrm.modrm.Write8(m_Bus, m_Registers.ds, Fetch8());
	};

	// C7 - MOV Ev, Iv
	m_Functions[0xc7] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, Fetch16());
	};

	// F7 - GRP3b Ev
	m_Functions[0xf7] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Group);
		switch (modrm.reg.group) {
			// TEST Ev Iv
			case 0: {
				uint16_t result = modrm.modrm.Read16(m_Bus, m_Registers.ds) & Fetch16();
				SetFlagByValue(Flags::SF, result & 0x8000);
				SetFlagByValue(Flags::ZF, result == 0);
				SetFlagByValue(Flags::PF, parity[result & 0xff]);
				ClearFlag(Flags::CF);
				ClearFlag(Flags::OF);
				break;
			}

			default: {
				InvalidOpcode();
			}
		}
	};

	// 75 - JNZ
	m_Functions[0x75] = [this]() {
		JumpRelative(!GetFlag(Flags::ZF), static_cast<int8_t>(Fetch8()));
	};

	// 33 - XOR Gv, Ev
	m_Functions[0x33] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		uint16_t result = modrm.reg.Read16() ^ modrm.modrm.Read16(m_Bus, m_Registers.ds);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, result);

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
		ClearFlag(Flags::CF);
		ClearFlag(Flags::OF);
	};

	// 85 - TEST Gv Ev
	m_Functions[0x85] = [this]() {
		ModRM modrm = FetchModRM(true, RegEncoding::Register16);
		uint16_t result = modrm.modrm.Read16(m_Bus, m_Registers.ds) & modrm.reg.Read16();

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
		ClearFlag(Flags::CF);
		ClearFlag(Flags::OF);
	};

	// E6 - OUT Ib, AL
	m_Functions[0xe6] = [this]() {
		m_Bus->WriteByteToPort(Fetch8(), m_Registers.al);
	};

	// E7 - OUT Ib, AX
	m_Functions[0xe7] = [this]() {
		m_Bus->WriteWordToPort(Fetch8(), m_Registers.ax);
	};

	// EE - OUT DX, AL
	m_Functions[0xee] = [this]() {
		m_Bus->WriteByteToPort(m_Registers.dx, m_Registers.al);
	};

	// EF - OUT DX, AX
	m_Functions[0xef] = [this]() {
		m_Bus->WriteWordToPort(m_Registers.dx, m_Registers.ax);
	};

	// 40 - INC AX
	m_Functions[0x40] = [this]() {
		uint16_t original = m_Registers.ax;
		m_Registers.ax++;
		
		SetFlagByValue(Flags::OF, m_Registers.ax == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.ax & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.ax == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.ax & 0xff]);
	};

	// 41 - INC CX
	m_Functions[0x41] = [this]() {
		uint16_t original = m_Registers.cx;
		m_Registers.cx++;
		
		SetFlagByValue(Flags::OF, m_Registers.cx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.cx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.cx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.cx & 0xff]);
	};

	// 42 - INC DX
	m_Functions[0x42] = [this]() {
		uint16_t original = m_Registers.dx;
		m_Registers.dx++;
		
		SetFlagByValue(Flags::OF, m_Registers.dx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.dx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.dx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.dx & 0xff]);
	};

	// 43 - INC BX
	m_Functions[0x43] = [this]() {
		uint16_t original = m_Registers.bx;
		m_Registers.bx++;
		
		SetFlagByValue(Flags::OF, m_Registers.bx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.bx & 0xff]);
	};

	// 44 - INC SP
	m_Functions[0x44] = [this]() {
		uint16_t original = m_Registers.sp;
		m_Registers.sp++;
		
		SetFlagByValue(Flags::OF, m_Registers.sp == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.sp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.sp == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.sp & 0xff]);
	};

	// 45 - INC BP
	m_Functions[0x45] = [this]() {
		uint16_t original = m_Registers.bp;
		m_Registers.bp++;
		
		SetFlagByValue(Flags::OF, m_Registers.bp == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bp == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.bp & 0xff]);
	};

	// 46 - INC SI
	m_Functions[0x46] = [this]() {
		uint16_t original = m_Registers.si;
		m_Registers.si++;
		
		SetFlagByValue(Flags::OF, m_Registers.si == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.si & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.si == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.si & 0xff]);
	};

	// 47 - INC DI
	m_Functions[0x47] = [this]() {
		uint16_t original = m_Registers.di;
		m_Registers.di++;
		
		SetFlagByValue(Flags::OF, m_Registers.di == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.di & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.di == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.di & 0xff]);
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

	// REG (group)
	else if (encoding == RegEncoding::Group) {
		result.reg.type = RegType::Raw;
		result.reg.group = reg;
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