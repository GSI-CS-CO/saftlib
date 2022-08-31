#include "Dice2.hpp"
#include <iostream>

namespace example {


Dice2::Dice2() {
	max2 = 6;
}

double Dice2::roll2() {
	std::cerr << "************ Dice2::roll2()" << std::endl; 
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