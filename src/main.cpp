#include "bus.hpp"
#include "cpu.hpp"

#include <print>
#include <memory>

int main() {
	auto bus = std::make_shared<xe86::Bus>("roms/GLABIOS_0.4.1_8T.ROM");
	xe86::CPU cpu(bus);
	cpu.Reset();

	while (true) {
		cpu.Step();
	}

	std::println("prank");
}