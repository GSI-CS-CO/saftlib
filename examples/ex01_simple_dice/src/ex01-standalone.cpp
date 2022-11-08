#include "Dice.hpp"

#include <iostream>
#include <saftbus/loop.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}
void dice_thrown_callback_sigc(int result) {
	std::cout << "SIGC: +++++++ Dice was thrown, result was " << result << std::endl;
}

int main(int argc, char *argv[]) {

	auto dice = ex01::Dice::create("/ex01/Dice");
	// connect the callback 
	dice->was_thrown = &dice_thrown_callback;
	dice->was_thrown_sigc.connect(sigc::ptr_fun(dice_thrown_callback_sigc));

	if (argc == 2) {	
		std::string command = argv[1];
		if (command == "once") {
			dice->throwOnce();
			saftbus::Loop::get_default().iteration(true);
			return 0;
		} 
		else if (command == "start") {
			dice->throwPeriodically(500);
		} 
		else if (command == "stop") {
			dice->stopThrowing();
			return 0;
		} else {
			std::cout << "usage: " << argv[0] << " once | start | stop " << std::endl;
			std::cout << "     without any arguments, the program waits for dice throws" << std::endl;
			return 0;
		}
	}

	while(true) {
		saftbus::Loop::get_default().iteration(true);
	}
	return 0;
}
