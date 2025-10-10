#include "bus.hpp"
#include <print>
#include <fstream>

using namespace xe86;

void MemoryArea::LoadFromFile(std::string_view filename) {
	std::ifstream file(filename.data(), std::ios::binary | std::ios::ate);
	if (!file) {
		std::println("failed to load file '{}'", filename);
		exit(1);
	}

	size_t size = file.tellg();
	if (size != m_Length) {
		std::println("expected size {}, got {}", m_Length, size);
		exit(1);
	}

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(m_Area.data()), size);
}

std::shared_ptr<MemoryArea> Bus::FindArea(Address20 address) {
	for (auto area : m_Memory) {
		if (address >= area->GetStartAddress() && address <= area->GetEndAddress()) {
			return area;
		}
	}

	return nullptr;
}

uint8_t Bus::ReadByte(Address20 address) {
	auto area = FindArea(address);
	if (!area) {
		std::println(stderr, "attempted to read from unknown memory area @ {:05x}", static_cast<uint32_t>(address));
		return 0;
	}

	return area->ReadByte(address - area->GetStartAddress());
}

void Bus::WriteByte(Address20 address, uint8_t byte) {
	auto area = FindArea(address);
	if (!area) {
		std::println(stderr, "attempted to write {:02x} to unknown memory area @ {:05x}",
			static_cast<uint8_t>(byte),
			static_cast<uint32_t>(address)
		);

		return;
	}

	area->WriteByte(address - area->GetStartAddress(), byte);
}