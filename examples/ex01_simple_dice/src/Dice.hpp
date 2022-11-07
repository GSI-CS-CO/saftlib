#ifndef DICE_HPP_
#define DICE_HPP_


#include <saftbus/loop.hpp>

#include <functional>
#include <memory>

#include <sigc++/sigc++.h>

namespace ex01 {

	class Dice {
		bool auto_throwing_enabled;
		saftbus::SourceHandle throw_timeout_source;
	public:
		/// @brief convenience function to allow similar creation compared to Proxy class
		/// @param object_path is ignored
		static std::shared_ptr<Dice> create(const std::string &object_path);

		Dice();
		~Dice();

		/// @brief throw dice once and return the result.
		// @saftbus-export
		int throwOnce();

		/// @brief start periodic throwing.
		// @saftbus-export
		void throwPeriodically(int interval_ms);

		/// @brief stop periodic throwing. 
		// @saftbus-export
		void stopThrowing();

		// @saftbus-export
		std::function<void(int result)> was_thrown;

		// @saftbus-export
		sigc::signal<void, int> was_thrown_sigc;
	};


}

#endif
