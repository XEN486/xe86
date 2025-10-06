#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "bus.hpp"
#include "component.hpp"

#include <vector>
#include <print>
#include <string>

namespace xe86 {
	class EmulatorState {
	public:
		EmulatorState(std::string_view bios_rom) : m_Bus(std::make_shared<Bus>(bios_rom)) {}
		EmulatorState(std::shared_ptr<Bus> bus) : m_Bus(bus) {}

		template <typename T>
		void AttachComponent() {
			auto component = std::make_unique<T>(m_Bus);
			std::println("emulator: adding new component '{}'", component->GetHumanName());
			m_Components.push_back(std::move(component));
		}

		void Reset() {
			for (auto& component : m_Components) {
				component->Reset();
			}
		}

		void Step() {
			for (auto& component : m_Components) {
				component->Step();
			}
		}

	private:
		std::shared_ptr<Bus> m_Bus;
		std::vector<std::unique_ptr<Component>> m_Components;
	};
}

#endif