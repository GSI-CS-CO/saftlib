#include "DiceCup.hpp"
#include "Dice.hpp"

#include <iostream>
#include <saftbus/loop.hpp>

void dice_thrown_callback(int result) {
	std::cout << "Dice was thrown, result was " << result << std::endl;
}

int main(int argc, char *argv[]) {

	auto dice_cup = ex02::DiceCup::create("/ex02/DiceCup");
	ex02::Dice* dice_listen;

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
				dice_listen = dice_cup->getDice(name);
				dice_listen->was_thrown = &dice_thrown_callback;
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else if (command == "start") {
			if (++i < argc) {
				std::string name = argv[i];
				dice_cup->getDice(name)->throwPeriodically(500);
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else if (command == "stop") {
			if (++i < argc) {
				std::string name = argv[i];
				dice_cup->getDice(name)->stopThrowing();
			} else {
				throw std::runtime_error("expect name after ListenTo");
			}
		} else {
			std::cout << "usage: " << argv[0] << " addDice6 <name> | addDice12 <name> | listen <name> | start <name> | stop <name> " << std::endl;
		}
	}

	if (dice_listen) {
		while(true) {
			saftbus::Loop::get_default().iteration(true);
		}
	}
	return 0;
}
