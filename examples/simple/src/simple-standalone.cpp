#include "Dice.hpp"

#include <iostream>

void use_dice_result(int result) {
	std::cerr << "Dice was thrown, result was " << result << std::endl;
}

int main() {
	simple::Dice dice;
	dice.was_thrown = &use_dice_result;
	dice.throwOnce();

	dice.throwPeriodically(500);
	saftbus::Loop::get_default().run();
	return 0;
}