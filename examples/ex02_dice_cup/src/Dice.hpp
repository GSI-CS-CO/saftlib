#ifndef DICE_HPP_
#define DICE_HPP_


#include <saftbus/loop.hpp>

#include <functional>
#include <memory>

namespace ex02 {

	class Dice {

		saftbus::Source *throw_timeout_source;
	public:
		Dice();
		virtual ~Dice();

		/// @brief throw dice once and return the result.
		// @saftbus-export
		virtual int throwOnce() = 0;

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
