#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

namespace xe86 {
	using Register16 = uint16_t;
	using Register8 = uint8_t;
	using SegmentRegister = uint16_t;
	using PortAddress16 = uint16_t;
	using PortAddress8 = uint8_t;

	struct Address20 {
		uint32_t value;

		Address20(SegmentRegister seg, uint16_t imm) : value(static_cast<uint32_t>(seg) * 16 + imm) {}
		Address20(uint32_t value) : value(value) {}

		operator uint32_t() const { return value & 0xfffff; }
	};
}

#endif