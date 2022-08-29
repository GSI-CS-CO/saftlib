#include "Dice.hpp"

namespace example {

Dice::Dice() {
	max = 6;
}

double Dice::roll() {
	return 1+rand()%max;
}

void Dice::setMax(int max) {
	this->max = max;
}
int Dice::getMax() {
	return max;
}

Dice2::Dice2() {
	max2 = 6;
}

double Dice2::roll2() {
	return 1+rand()%max2;
}

void Dice2::setMax2(int max) {
	this->max2= max2;
}
int Dice2::getMax2() {
	return max2;
}
bool Dice2::dont_export() {
	return false;
}

}