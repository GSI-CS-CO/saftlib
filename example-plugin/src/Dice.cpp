#include "Dice.hpp"

namespace example {

int Dice::roll() {
	return 1+rand()%6;
}

}