#include <iostream>
#include "Dice.hpp"
#include "Dice2.hpp"

namespace example {

	class SpecialDice :public Dice , public Dice2 {
	public:
		SpecialDice();
		// @saftbus-export
		double rollf();
	};
}