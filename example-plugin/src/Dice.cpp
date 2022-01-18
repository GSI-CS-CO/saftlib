#include "Dice.hpp"

namespace example {

Dice::Dice() {
	max = 6;
}

int Dice::roll() {
	return 1+rand()%max;
}

void Dice::setMax(int max) {
	this->max = max;
}
int Dice::getMax() {
	return max;
}

}