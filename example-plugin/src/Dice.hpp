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


	class Dice2 {
	public:
		Dice2();
		// @saftbus-export
		double roll2();
		// @saftbus-export
		void setMax2(int max);
		// @saftbus-export
		int getMax2();
		// normal comment
		bool dont_export();
	private:
		int max2;
	};


}