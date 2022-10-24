#include <iostream>
#include <functional>

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
		// @saftbus-signal
		std::function<void(int a,int b,int c)> signal1;
	private:
		int max;
	};




}