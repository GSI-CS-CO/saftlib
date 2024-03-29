#ifndef DICE_HPP_
#define DICE_HPP_


#include <saftbus/loop.hpp>

#include <functional>
#include <memory>

namespace ex02 {

	class Dice {
		bool auto_throwing_enabled;
		saftbus::SourceHandle throw_timeout_source;
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

		// @saftbus-export
		std::function<void(int result)> was_thrown;
	};


}

#endif
