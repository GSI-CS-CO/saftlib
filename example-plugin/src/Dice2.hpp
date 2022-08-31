#include <iostream>

namespace example {


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
		// @saftbus-export
		void trigger_exception();
	private:
		int max2;
	};


}