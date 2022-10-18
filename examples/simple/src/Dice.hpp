#ifndef DICE_HPP_
#define DICE_HPP_

#include <functional>
#include <saftbus/loop.hpp>

namespace simple {

	class Dice {
		saftbus::Source *throw_timeout_source;
	public:
		/// @brief throw dice once and return the result.
		// @saftbus-export
		int throwOnce();
		/// @brief start periodic throwing.
		// @saftbus-export
		void throwPeriodically(int interval_ms);
		/// @brief stop periodic throwing. 
		// @saftbus-export
		void stopThrowing();
		// @saftbus-signal
		std::function<void(int result)> was_thrown;
	};


}

#endif
