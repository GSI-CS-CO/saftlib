#include <iostream>

namespace example {

	class Dice {
	public:
		Dice();
		// @saftbus-export
		double roll();
		// @saftbus-export
		void setMax(int max);
		// @saftbus-export
		int getMax();
	private:
		int max;
	};




}