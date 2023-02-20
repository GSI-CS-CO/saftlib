#ifndef EX00_DICE_HPP
#define EX00_DICE_HPP
#include <sigc++/sigc++.h>
namespace ex00 {
	class Dice {
	public:
		/// @brief throw dice once and return the result.
		/// @return an integer value in the range [1..6]
		/// special comment in the next line tells saftbus-gen to expose this method
		// @saftbus-export
		void Throw();
		// @saftbus-export
		sigc::signal<void, int> Result;
	};
}

#endif
