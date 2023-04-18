#ifndef EX00_DICE_HPP
#define EX00_DICE_HPP
#include <sigc++/sigc++.h>
#include <functional>
namespace ex00 {
	class Dice {
	public:
		/// @brief throw dice once and return the result.
		/// @return an integer value in the range [1..6]
		/// special comment in the next line tells saftbus-gen to expose this method
		// @saftbus-export
		void Throw();
		/// @brief a signal using sigc++ library
		// @saftbus-export
		sigc::signal<void, int> SigResult;
		/// @brief a signal using std::function
		// @saftbus-export
		std::function<void(int result)> FunResult;
	};
}

#endif
