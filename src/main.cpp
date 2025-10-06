#include "emulator.hpp"
#include "cpu.hpp"

#include <print>
#include <memory>

int main() {
	xe86::EmulatorState emulator("roms/GLABIOS_0.4.1_8T.ROM");
	emulator.AttachComponent<xe86::CPU>();

	emulator.Reset();
	while (true) {
		emulator.Step();
	}
}