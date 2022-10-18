#include "Dice.hpp"

#include <iostream>
#include <saftbus/loop.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}

int main(int argc, char *argv[]) {

	auto dice = simple::Dice::create("/simple/Dice");
	// connect the callback 
	dice->was_thrown = &dice_thrown_callback;

	if (argc == 2) {	
		std::string command = argv[1];
		if (command == "once") {
			dice->throwOnce();
			saftbus::Loop::get_default().iteration(true);
			return 0;
		} 
		else if (command == "start") {
			dice->throwPeriodically(1500);
		} 
		else if (command == "stop") {
			dice->stopThrowing();
			return 0;
		} else {
			std::cout << "usage: " << argv[0] << " once | start | stop " << std::endl;
			return 0;
		}
	}

	while(true) {
		saftbus::Loop::get_default().iteration(true);
	}
	return 0;
}
