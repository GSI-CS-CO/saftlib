#include "Dice.hpp"
#include <iostream>

namespace example {

Dice::Dice() {
	max = 6;
}

double Dice::roll() {
	std::cerr << "************ Dice::roll()" << std::endl; 
	return 1+rand()%max;
}

void Dice::setMax(int max) {
	this->max = max;
}
int Dice::getMax() {
	return max;
}

}