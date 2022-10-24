#include "Dice.hpp"

#include <saftbus/error.hpp>

namespace ex02 {

	Dice::Dice() 
		: throw_timeout_source(nullptr)
	{
	}

	Dice::~Dice() {
		stopThrowing();
	}

	void Dice::throwPeriodically(int period_ms) {
		if (throw_timeout_source) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "periodic throwing is already enabled");
		}
		throw_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&Dice::throwOnce, this), 
			std::chrono::milliseconds(period_ms),
			std::chrono::milliseconds(period_ms)
			);
	}

	void Dice::stopThrowing() {
		if (throw_timeout_source != nullptr) {
			saftbus::Loop::get_default().remove(throw_timeout_source);
			throw_timeout_source = nullptr;
		}
	}


}