#include "DiceCup_Proxy.hpp"
#include "Dice_Proxy.hpp"

#include <iostream>
#include <saftbus/loop.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}

int main(int argc, char *argv[]) {

	auto dice_cup = ex02::DiceCup_Proxy::create("/ex02/DiceCup");
	std::shared_ptr<ex02::Dice_Proxy> dice_listen;

	for (int i = 1; i < argc; ++i) {
		std::string command = argv[i];
		if (command == "addDice6") {
			if (++i < argc) {
				std::string name = argv[i];
				dice_cup->addDice6(name);
			} else {
				throw std::runtime_error("expect name after AddDice6");
			}
		} else if (command == "addDice12") {
			if (++i < argc) {
				std::string name = argv[i];
				dice_cup->addDice12(name);
			} else {
				throw std::runtime_error("expect name after AddDice12");
			}
		} else if (command == "listen") {
			if (++i < argc) {
				std::string name = argv[i];
				dice_listen = ex02::Dice_Proxy::create(name);
				dice_listen->was_thrown = &dice_thrown_callback;
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else if (command == "start") {
			if (++i < argc) {
				std::string name = argv[i];
				ex02::Dice_Proxy::create(name)->throwPeriodically(500);
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else if (command == "stop") {
			if (++i < argc) {
				std::string name = argv[i];
				ex02::Dice_Proxy::create(name)->stopThrowing();
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else {
			std::cout << "usage: " << argv[0] << " addDice6 <name> | addDice12 <name> | listen <name> | start <name> | stop <name> " << std::endl;
		}
	}

	if (dice_listen) {
		while(true) {
			saftbus::SignalGroup::get_global().wait_for_signal();
		}
	}
	return 0;
}
