#include "Dice.hpp"

#include <saftbus/error.hpp>

namespace ex02 {

	Dice::Dice() 
		: auto_throwing_enabled(false)
	{
	}

	Dice::~Dice() {
		std::cerr << "Dice::~Dice()" << std::endl;
		stopThrowing();
	}

	void Dice::throwPeriodically(int period_ms) {
		if (auto_throwing_enabled) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "periodic throwing is already enabled");
		}
		throw_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&Dice::throwOnce, this), 
			std::chrono::milliseconds(period_ms),
			std::chrono::milliseconds(period_ms)
			);
		auto_throwing_enabled = true;
	}

	void Dice::stopThrowing() {
		saftbus::Loop::get_default().remove(throw_timeout_source);
		auto_throwing_enabled = false;
	}


}