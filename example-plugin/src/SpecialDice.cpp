#include "SpecialDice.hpp"

namespace example {

SpecialDice::SpecialDice() : Dice() {

}


double SpecialDice::rollf() {
	std::cerr << "************ SpecialDice::rollf()" << std::endl; 
	return 1+1.0*getMax()*rand()/RAND_MAX;
}

}