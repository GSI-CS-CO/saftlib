#include "Dice_Proxy.hpp"

#include <iostream>
#include <saftbus/client.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}

int main(int argc, char *argv[]) {

	auto dice = simple::Dice_Proxy::create("/simple/Dice");
	// connect the callback 
	dice->was_thrown = &dice_thrown_callback;

	if (argc == 2) {	
		std::string command = argv[1];
		if (command == "once") {
			dice->throwOnce();
			saftbus::SignalGroup().get_global().wait_for_signal();
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
			std::cout << "     without any arguments, the program waits for dice throws" << std::endl;
			return 0;
		}
	}

	while(true) {
		saftbus::SignalGroup().get_global().wait_for_signal();
	}
	return 0;
}
