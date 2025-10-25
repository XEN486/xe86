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
	// JMP Ap
	m_Functions[0xea] = [this]() {
		// if we update CS and IP directly then it will read from the wrong address
		uint16_t new_ip = Fetch16();
		uint16_t new_cs = Fetch16();

		m_Registers.ip = new_ip;
		m_Registers.cs = new_cs;
	};

	// CLI
	m_Functions[0xfa] = [this]() {
		ClearFlag(Flags::IF); // interrupt flag
	};

	// CLD
	m_Functions[0xfc] = [this]() {
		ClearFlag(Flags::DF); // direction flag
	};

	// MOV Eb, Gb
	m_Functions[0x88] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		modrm.modrm.Write8(m_Bus, m_Registers.ds, modrm.reg.Read8());
	};

	// MOV Ev, Gv
	m_Functions[0x89] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, modrm.reg.Read16());
	};

	// MOV Gb, Eb
	m_Functions[0x8a] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		modrm.reg.Write8(modrm.modrm.Read8(m_Bus, m_Registers.ds));
	};

	// MOV Gv, Ev
	m_Functions[0x8b] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus, m_Registers.ds)); // TODO: the segment can be changed using a segment prefix
	};

	// MOV Ew, Sw
	m_Functions[0x8c] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Segment);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, modrm.reg.Read16());
	};

	// MOV Sw, Ew
	m_Functions[0x8e] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Segment);
		modrm.reg.Write16(modrm.modrm.Read16(m_Bus, m_Registers.ds));
	};

	// MOV AL, Ob
	m_Functions[0xa0] = [this]() {
		m_Registers.al = m_Bus->ReadByte((m_Registers.ds * 0x10) + Fetch16());
	};

	// MOV AX, Ov
	m_Functions[0xa1] = [this]() {
		m_Registers.ax = m_Bus->ReadWord((m_Registers.ds * 0x10) + Fetch16());
	};

	// MOV Ob, AL
	m_Functions[0xa2] = [this]() {
		uint16_t addr = (m_Registers.ds * 0x10) + Fetch16();
		m_Bus->WriteByte(addr, m_Registers.al);
	};

	// MOV Ov, AX
	m_Functions[0xa3] = [this]() {
		uint16_t addr = (m_Registers.ds * 0x10) + Fetch16();
		m_Bus->WriteWord(addr, m_Registers.ax);
	};

	// MOVSB
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

	// MOVSW
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

	// MOV AL, Ib
	m_Functions[0xb0] = [this]() {
		m_Registers.al = Fetch8();
	};

	// MOV CL, Ib
	m_Functions[0xb1] = [this]() {
		m_Registers.cl = Fetch8();
	};

	// MOV DL, Ib
	m_Functions[0xb2] = [this]() {
		m_Registers.dl = Fetch8();
	};

	// MOV BL, Ib
	m_Functions[0xb3] = [this]() {
		m_Registers.bl = Fetch8();
	};

	// MOV AH, Ib
	m_Functions[0xb4] = [this]() {
		m_Registers.ah = Fetch8();
	};

	// MOV CH, Ib
	m_Functions[0xb5] = [this]() {
		m_Registers.ch = Fetch8();
	};

	// MOV DH, Ib
	m_Functions[0xb6] = [this]() {
		m_Registers.dh = Fetch8();
	};

	// MOV BH, Ib
	m_Functions[0xb7] = [this]() {
		m_Registers.bh = Fetch8();
	};

	// MOV AX, Iv
	m_Functions[0xb8] = [this]() {
		m_Registers.ax = Fetch16();
	};

	// MOV CX, Iv
	m_Functions[0xb9] = [this]() {
		m_Registers.cx = Fetch16();
	};

	// MOV DX, Iv
	m_Functions[0xba] = [this]() {
		m_Registers.dx = Fetch16();
	};

	// MOV BX, Iv
	m_Functions[0xbb] = [this]() {
		m_Registers.bx = Fetch16();
	};

	// MOV SP, Iv
	m_Functions[0xbc] = [this]() {
		m_Registers.sp = Fetch16();
	};

	// MOV BP, Iv
	m_Functions[0xbd] = [this]() {
		m_Registers.bp = Fetch16();
	};

	// MOV SI, Iv
	m_Functions[0xbe] = [this]() {
		m_Registers.si = Fetch16();
	};

	// MOV DI, Iv
	m_Functions[0xbf] = [this]() {
		m_Registers.di = Fetch16();
	};

	// MOV Eb, Ib
	m_Functions[0xc6] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		modrm.modrm.Write8(m_Bus, m_Registers.ds, Fetch8());
	};

	// MOV Ev, Iv
	m_Functions[0xc7] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		modrm.modrm.Write16(m_Bus, m_Registers.ds, Fetch16());
	};

	// GRP3b Ev
	m_Functions[0xf7] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Group);

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

	// XOR Gv, Ev
	m_Functions[0x33] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.reg.Read16() ^ modrm.modrm.Read16(m_Bus, m_Registers.ds);
		modrm.reg.Write16(result);

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
		ClearFlag(Flags::CF);
		ClearFlag(Flags::OF);
	};

	// TEST Gv Ev
	m_Functions[0x85] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.modrm.Read16(m_Bus, m_Registers.ds) & modrm.reg.Read16();

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
		ClearFlag(Flags::CF);
		ClearFlag(Flags::OF);
	};

	// OUT Ib, AL
	m_Functions[0xe6] = [this]() {
		m_Bus->WriteByteToPort(Fetch8(), m_Registers.al);
	};

	// OUT Ib, AX
	m_Functions[0xe7] = [this]() {
		m_Bus->WriteWordToPort(Fetch8(), m_Registers.ax);
	};

	// OUT DX, AL
	m_Functions[0xee] = [this]() {
		m_Bus->WriteByteToPort(m_Registers.dx, m_Registers.al);
	};

	// OUT DX, AX
	m_Functions[0xef] = [this]() {
		m_Bus->WriteWordToPort(m_Registers.dx, m_Registers.ax);
	};

	// INC AX
	m_Functions[0x40] = [this]() {
		uint16_t original = m_Registers.ax;
		m_Registers.ax++;
		
		SetFlagByValue(Flags::OF, m_Registers.ax == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.ax & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.ax == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.ax & 0xff]);
	};

	// INC CX
	m_Functions[0x41] = [this]() {
		uint16_t original = m_Registers.cx;
		m_Registers.cx++;
		
		SetFlagByValue(Flags::OF, m_Registers.cx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.cx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.cx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.cx & 0xff]);
	};

	// INC DX
	m_Functions[0x42] = [this]() {
		uint16_t original = m_Registers.dx;
		m_Registers.dx++;
		
		SetFlagByValue(Flags::OF, m_Registers.dx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.dx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.dx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.dx & 0xff]);
	};

	// INC BX
	m_Functions[0x43] = [this]() {
		uint16_t original = m_Registers.bx;
		m_Registers.bx++;
		
		SetFlagByValue(Flags::OF, m_Registers.bx == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bx == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.bx & 0xff]);
	};

	// INC SP
	m_Functions[0x44] = [this]() {
		uint16_t original = m_Registers.sp;
		m_Registers.sp++;
		
		SetFlagByValue(Flags::OF, m_Registers.sp == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.sp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.sp == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.sp & 0xff]);
	};

	// INC BP
	m_Functions[0x45] = [this]() {
		uint16_t original = m_Registers.bp;
		m_Registers.bp++;
		
		SetFlagByValue(Flags::OF, m_Registers.bp == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bp == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.bp & 0xff]);
	};

	// INC SI
	m_Functions[0x46] = [this]() {
		uint16_t original = m_Registers.si;
		m_Registers.si++;
		
		SetFlagByValue(Flags::OF, m_Registers.si == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.si & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.si == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.si & 0xff]);
	};

	// INC DI
	m_Functions[0x47] = [this]() {
		uint16_t original = m_Registers.di;
		m_Registers.di++;
		
		SetFlagByValue(Flags::OF, m_Registers.di == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.di & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.di == 0);
		SetFlagByValue(Flags::AF, ((original & 0x0f) + 1) > 0x0f);
		SetFlagByValue(Flags::PF, parity[m_Registers.di & 0xff]);
	};

	// DEC AX
	m_Functions[0x48] = [this]() {
		uint16_t original = m_Registers.ax;
		m_Registers.ax--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.ax & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.ax == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.ax & 0xff]);
	};

	// DEC CX
	m_Functions[0x49] = [this]() {
		uint16_t original = m_Registers.cx;
		m_Registers.cx--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.cx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.cx == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.cx & 0xff]);
	};

	// DEC DX
	m_Functions[0x4a] = [this]() {
		uint16_t original = m_Registers.dx;
		m_Registers.dx--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.dx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.dx == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.dx & 0xff]);
	};

	// DEC BX
	m_Functions[0x4b] = [this]() {
		uint16_t original = m_Registers.bx;
		m_Registers.bx--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bx & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bx == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.bx & 0xff]);
	};

	// DEC SP
	m_Functions[0x4c] = [this]() {
		uint16_t original = m_Registers.sp;
		m_Registers.sp--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.sp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.sp == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.sp & 0xff]);
	};

	// DEC BP
	m_Functions[0x4d] = [this]() {
		uint16_t original = m_Registers.bp;
		m_Registers.bp--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.bp & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.bp == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.bp & 0xff]);
	};

	// DEC SI
	m_Functions[0x4e] = [this]() {
		uint16_t original = m_Registers.si;
		m_Registers.si--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.si & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.si == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.si & 0xff]);
	};

	// DEC DI
	m_Functions[0x4f] = [this]() {
		uint16_t original = m_Registers.di;
		m_Registers.di--;
		
		SetFlagByValue(Flags::OF, original == 0x8000);
		SetFlagByValue(Flags::SF, m_Registers.di & 0x8000);
		SetFlagByValue(Flags::ZF, m_Registers.di == 0);
		SetFlagByValue(Flags::AF, (original & 0x0f) == 0);
		SetFlagByValue(Flags::PF, parity[m_Registers.di & 0xff]);
	};

	// GRP1 Ev, Iv
	m_Functions[0x81] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Group);
		switch (modrm.reg.group) {
			// CMP Ev, Iv
			case 7: {
				uint16_t ev = modrm.modrm.Read16(m_Bus, m_Registers.ds);
				uint16_t iv = Fetch16();
				uint16_t result = ev - iv;

				SetFlagByValue(Flags::SF, result & 0x8000);
				SetFlagByValue(Flags::ZF, result == 0);
				SetFlagByValue(Flags::PF, parity[result & 0xff]);
				SetFlagByValue(Flags::CF, ev < iv);
				SetFlagByValue(Flags::OF, ((ev ^ iv) & 0x8000) != 0 && ((ev ^ result) & 0x8000) != 0);
				SetFlagByValue(Flags::AF, (ev & 0x0f) < (iv & 0x0f));

				break;
			}

			default: {
				InvalidOpcode();
			}
		}
	};

	// IN AL, Ib
	m_Functions[0xe4] = [this]() {
		m_Registers.al = m_Bus->ReadByteFromPort(Fetch8());
	};

	// IN AX, Ib
	m_Functions[0xe5] = [this]() {
		m_Registers.ax = m_Bus->ReadWordFromPort(Fetch8());
	};

	// IN AL, DX
	m_Functions[0xec] = [this]() {
		m_Registers.al = m_Bus->ReadByteFromPort(m_Registers.dx);
	};

	// IN AX, DX
	m_Functions[0xed] = [this]() {
		m_Registers.ax = m_Bus->ReadWordFromPort(m_Registers.dx);
	};

	// AND Eb, Gb
	m_Functions[0x20] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t result = modrm.modrm.Read8(m_Bus, m_Registers.ds) & modrm.reg.Read8();
		modrm.modrm.Write8(m_Bus, m_Registers.ds, result);

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// AND Ev, Gv
	m_Functions[0x21] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.modrm.Read16(m_Bus, m_Registers.ds) & modrm.reg.Read16();
		modrm.modrm.Write16(m_Bus, m_Registers.ds, result);
		
		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// AND Gb, Eb
	m_Functions[0x22] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t result = modrm.reg.Read8() & modrm.modrm.Read8(m_Bus, m_Registers.ds);
		modrm.reg.Write8(result);

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// AND Gv, Ev
	m_Functions[0x23] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.reg.Read16() & modrm.modrm.Read16(m_Bus, m_Registers.ds);
		modrm.reg.Write16(result);

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// AND AL, Ib
	m_Functions[0x24] = [this]() {
		uint8_t result = m_Registers.al & Fetch8();
		m_Registers.al = result;

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// AND AX, Iv
	m_Functions[0x25] = [this]() {
		uint16_t result = m_Registers.ax & Fetch16();
		m_Registers.ax = result;

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR Eb, Gb
	m_Functions[0x08] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t result = modrm.modrm.Read8(m_Bus, m_Registers.ds) | modrm.reg.Read8();
		modrm.modrm.Write8(m_Bus, m_Registers.ds, result);

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR Ev, Gv
	m_Functions[0x09] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.modrm.Read16(m_Bus, m_Registers.ds) | modrm.reg.Read16();
		modrm.modrm.Write16(m_Bus, m_Registers.ds, result);
		
		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR Gb, Eb
	m_Functions[0x0a] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t result = modrm.reg.Read8() | modrm.modrm.Read8(m_Bus, m_Registers.ds);
		modrm.reg.Write8(result);

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR Gv, Ev
	m_Functions[0x0b] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t result = modrm.reg.Read16() | modrm.modrm.Read16(m_Bus, m_Registers.ds);
		modrm.reg.Write16(result);

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR AL, Ib
	m_Functions[0x0c] = [this]() {
		uint8_t result = m_Registers.al | Fetch8();
		m_Registers.al = result;

		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// OR AX, Iv
	m_Functions[0x0d] = [this]() {
		uint16_t result = m_Registers.ax | Fetch16();
		m_Registers.ax = result;

		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::PF, parity[result]);

		ClearFlag(Flags::OF);
		ClearFlag(Flags::CF);
	};

	// JO
	m_Functions[0x70] = [this]() {
		JumpRelative(GetFlag(Flags::OF), static_cast<int8_t>(Fetch8()));
	};

	// JNO
	m_Functions[0x71] = [this]() {
		JumpRelative(!GetFlag(Flags::OF), static_cast<int8_t>(Fetch8()));
	};

	// JB
	m_Functions[0x72] = [this]() {
		JumpRelative(GetFlag(Flags::CF), static_cast<int8_t>(Fetch8()));
	};

	// JNB
	m_Functions[0x73] = [this]() {
		JumpRelative(!GetFlag(Flags::CF), static_cast<int8_t>(Fetch8()));
	};

	// JZ
	m_Functions[0x74] = [this]() {
		JumpRelative(GetFlag(Flags::ZF), static_cast<int8_t>(Fetch8()));
	};

	// JNZ
	m_Functions[0x75] = [this]() {
		JumpRelative(!GetFlag(Flags::ZF), static_cast<int8_t>(Fetch8()));
	};

	// JBE
	m_Functions[0x76] = [this]() {
		JumpRelative(GetFlag(Flags::CF) || GetFlag(Flags::ZF), static_cast<int8_t>(Fetch8()));
	};

	// JA
	m_Functions[0x77] = [this]() {
		JumpRelative(!GetFlag(Flags::CF) && !GetFlag(Flags::ZF), static_cast<int8_t>(Fetch8()));
	};

	// JS
	m_Functions[0x78] = [this]() {
		JumpRelative(GetFlag(Flags::SF), static_cast<int8_t>(Fetch8()));
	};

	// JNS
	m_Functions[0x79] = [this]() {
		JumpRelative(!GetFlag(Flags::SF), static_cast<int8_t>(Fetch8()));
	};

	// JPE
	m_Functions[0x7a] = [this]() {
		JumpRelative(GetFlag(Flags::PF), static_cast<int8_t>(Fetch8()));
	};

	// JPO
	m_Functions[0x7b] = [this]() {
		JumpRelative(!GetFlag(Flags::PF), static_cast<int8_t>(Fetch8()));
	};

	// JL
	m_Functions[0x7c] = [this]() {
		JumpRelative(GetFlag(Flags::SF) != GetFlag(Flags::OF), static_cast<int8_t>(Fetch8()));
	};

	// JGE
	m_Functions[0x7d] = [this]() {
		JumpRelative(GetFlag(Flags::SF) == GetFlag(Flags::OF), static_cast<int8_t>(Fetch8()));
	};

	// JLE
	m_Functions[0x7e] = [this]() {
		JumpRelative(GetFlag(Flags::ZF) || (GetFlag(Flags::SF) != GetFlag(Flags::OF)), static_cast<int8_t>(Fetch8()));
	};

	// JG
	m_Functions[0x7f] = [this]() {
		JumpRelative(!GetFlag(Flags::ZF) && (GetFlag(Flags::SF) == GetFlag(Flags::OF)), static_cast<int8_t>(Fetch8()));
	};

	// LODSB
	m_Functions[0xac] = [this]() {
		m_Registers.al = m_Bus->ReadByte((m_Registers.ds * 0x10) + m_Registers.si);

		if (!GetFlag(Flags::DF)) {
			m_Registers.si++;
		} else {
			m_Registers.si--;
		}
	};

	// LODSW
	m_Functions[0xad] = [this]() {
		m_Registers.ax = m_Bus->ReadWord((m_Registers.ds * 0x10) + m_Registers.si);
		
		if (!GetFlag(Flags::DF)) {
			m_Registers.si += 2;
		} else {
			m_Registers.si -= 2;
		}
	};

	// ADD Eb, Gb
	m_Functions[0x00] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t src1 = modrm.modrm.Read8(m_Bus, m_Registers.ds);
		uint8_t src2 = modrm.reg.Read8();
		uint8_t result = src1 + src2;

		modrm.modrm.Write8(m_Bus, m_Registers.ds, result);

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x80) == 0 && ((src1 ^ result) & 0x80) != 0);
		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result]);
	};

	// ADD Ev, Gv
	m_Functions[0x01] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t src1 = modrm.modrm.Read16(m_Bus, m_Registers.ds);
		uint16_t src2 = modrm.reg.Read16();
		uint16_t result = src1 + src2;

		modrm.modrm.Write16(m_Bus, m_Registers.ds, result);

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x8000) == 0 && ((src1 ^ result) & 0x8000) != 0);
		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
	};

	// ADD Gb, Eb
	m_Functions[0x02] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register8);
		uint8_t src1 = modrm.reg.Read8();
		uint8_t src2 = modrm.modrm.Read8(m_Bus, m_Registers.ds);
		uint8_t result = src1 + src2;

		modrm.reg.Write8(result);

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x80) == 0 && ((src1 ^ result) & 0x80) != 0);
		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result]);
	};

	// ADD Gv, Ev
	m_Functions[0x03] = [this]() {
		ModRM modrm = FetchModRM(RegEncoding::Register16);
		uint16_t src1 = modrm.reg.Read16();
		uint16_t src2 = modrm.modrm.Read16(m_Bus, m_Registers.ds);
		uint16_t result = src1 + src2;

		modrm.reg.Write16(result);

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x8000) == 0 && ((src1 ^ result) & 0x8000) != 0);
		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
	};

	// ADD AL, Ib
	m_Functions[0x04] = [this]() {
		uint8_t src1 = m_Registers.al;
		uint8_t src2 = Fetch8();
		uint8_t result = src1 + src2;

		m_Registers.al = result;

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x80) == 0 && ((src1 ^ result) & 0x80) != 0);
		SetFlagByValue(Flags::SF, result & 0x80);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result]);
	};

	// ADD AX, Iv
	m_Functions[0x05] = [this]() {
		uint16_t src1 = m_Registers.ax;
		uint16_t src2 = Fetch16();
		uint16_t result = src1 + src2;

		m_Registers.ax = result;

		SetFlagByValue(Flags::CF, result < src1);
		SetFlagByValue(Flags::OF, ((src1 ^ src2) & 0x8000) == 0 && ((src1 ^ result) & 0x8000) != 0);
		SetFlagByValue(Flags::SF, result & 0x8000);
		SetFlagByValue(Flags::ZF, result == 0);
		SetFlagByValue(Flags::AF, ((src1 & 0x0f) + (src2 & 0x0f)) > 0x0f);
		SetFlagByValue(Flags::PF, parity[result & 0xff]);
	};

	// LOOP Jb
	m_Functions[0xe2] = [this]() {
		m_Registers.cx--;
		JumpRelative(m_Registers.cx != 0, static_cast<int8_t>(Fetch8()));
	};

	// JMP Jb
	m_Functions[0xeb] = [this]() {
		JumpRelative(true, static_cast<int8_t>(Fetch8()));
	};

	// JMP Jv
	m_Functions[0xe9] = [this]() {
		JumpRelative16(true, static_cast<int16_t>(Fetch16()));
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
			case 0b000: result.reg.reg16 = &m_Registers.es; break;
			case 0b001: result.reg.reg16 = &m_Registers.cs; break;
			case 0b010: result.reg.reg16 = &m_Registers.ss; break;
			case 0b011: result.reg.reg16 = &m_Registers.ds; break;
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
		int8_t disp = static_cast<int8_t>(Fetch8());
		switch (rm) {
			case 0b000: result.modrm.addr = m_Registers.bx + m_Registers.si + disp; break;
			case 0b001: result.modrm.addr = m_Registers.bx + m_Registers.di + disp; break;
			case 0b010: result.modrm.addr = m_Registers.bp + m_Registers.si + disp; break;
			case 0b011: result.modrm.addr = m_Registers.bp + m_Registers.di + disp; break;
			case 0b100: result.modrm.addr = m_Registers.si + disp; break;
			case 0b101: result.modrm.addr = m_Registers.di + disp; break;
			case 0b110: result.modrm.addr = m_Registers.bp + disp; break;
			case 0b111: result.modrm.addr = m_Registers.bx + disp; break;
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
	//std::println("{:02x}", opcode);
	m_Functions[opcode]();
}