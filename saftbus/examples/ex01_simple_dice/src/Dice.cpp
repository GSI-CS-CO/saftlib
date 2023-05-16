#include "Dice.hpp"

#include <saftbus/error.hpp>

namespace ex01 {

	Dice::Dice() 
		: auto_throwing_enabled(false)
		, throw_timeout_source()
	{}

	Dice::~Dice() {
		stopThrowing();
	}

	std::shared_ptr<Dice> Dice::create(const std::string &object_path) {
		return std::make_shared<Dice>();
	}

	int Dice::throwOnce() {
		int result = rand()%6+1;
		if (was_thrown) {
			was_thrown(result);
		}
		was_thrown_sigc(result);
		return result;
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

	MyType Dice::passthrough(const MyType &val) {
		return val;
	}
	int Dice::passthrough2(const int &val) {
		return val;
	}


}