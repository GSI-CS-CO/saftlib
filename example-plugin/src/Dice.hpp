#include <iostream>

namespace example {

	class Dice {
	public:
		Dice();
		// @saftbus-export
		int roll();
		// @saftbus-export
		void setMax(int max);
		// @saftbus-export
		int getMax();
	private:
		int max;
	};

}