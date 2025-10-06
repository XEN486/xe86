#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "bus.hpp"
#include <memory>
#include <string>

namespace xe86 { 
	class Component {
	public:
		virtual ~Component() = default;
		Component(std::shared_ptr<Bus> bus, std::string human_name) : m_Bus(bus), m_HumanName(human_name) {}
		Component(std::shared_ptr<Bus> bus) : m_Bus(bus), m_HumanName("Unknown Component") {}

	public:
		virtual void Reset() = 0;
		virtual void Step() = 0;

	public:
		std::string_view GetHumanName() const {
			return m_HumanName;
		}

	protected:
		std::shared_ptr<Bus> m_Bus;
		std::string m_HumanName;
	};
}

#endif