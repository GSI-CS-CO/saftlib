#include "Dice.hpp"

#include <saftbus/loop.hpp>

namespace simple {


	int Dice::throwOnce() {
		int result = rand()%6+1;
		if (was_thrown) {
			was_thrown(result);
		}
		return result;
	}

	void Dice::throwPeriodically(int period_ms) {
		throw_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&Dice::throwOnce, this), 
			std::chrono::milliseconds(period_ms),
			std::chrono::milliseconds(period_ms)
			);
	}

	void Dice::stopThrowing() {
		saftbus::Loop::get_default().remove(throw_timeout_source);
	}

}