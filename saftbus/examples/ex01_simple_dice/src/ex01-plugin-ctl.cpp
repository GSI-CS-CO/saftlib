#include "Dice_Proxy.hpp"

#include <iostream>
#include <chrono>
#include <saftbus/client.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}
void dice_thrown_callback_sigc(int result) {
	std::cout << "SIGC: +++++++ Dice was thrown, result was " << result << std::endl;
}

void dice_service_destroyed(bool *run_loop) {
	*run_loop = false;
}

int main(int argc, char *argv[]) {

	auto dice = ex01::Dice_Proxy::create("/ex01/Dice");
	// connect the callback 
	dice->was_thrown = &dice_thrown_callback;
	dice->was_thrown_sigc.connect(sigc::ptr_fun(dice_thrown_callback_sigc));

	if (argc == 2) {	
		std::string command = argv[1];
		if (command == "once") {
			dice->throwOnce();
			saftbus::SignalGroup().get_global().wait_for_signal();
			return 0;
		} 
		else if (command == "start") {
			dice->throwPeriodically(500);
		} 
		else if (command == "stop") {
			dice->stopThrowing();
			return 0;
		} else if (command == "passthrough") {
			ex01::MyType val;
			val.type = command;
			val.name = "welt";
			for (int i = 0 ; i < 10000; ++i) {
				auto start = std::chrono::steady_clock::now();
				int x = dice->passthrough2(42);
				auto stop = std::chrono::steady_clock::now();
				ex01::MyType result = dice->passthrough(val);
				std::cout << i << " " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

			}
			return 0;
		} else {
			std::cout << "usage: " << argv[0] << " once | start | stop " << std::endl;
			std::cout << "     without any arguments, the program waits for dice throws" << std::endl;
			return 0;
		}
	}

	bool run_loop = true;
	dice->was_destroyed.connect(sigc::bind(sigc::ptr_fun(&dice_service_destroyed), &run_loop));
	while(run_loop) {
		saftbus::SignalGroup().get_global().wait_for_signal();
	}
	return 0;
}
