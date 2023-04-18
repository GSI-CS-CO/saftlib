#include "Dice.hpp"
#include "Dice_Service.hpp"
#include <saftbus/error.hpp>
#include <vector>
#include <string>

std::unique_ptr<ex00::Dice> dice;

void destroy_callback() {
	dice.reset();
}

extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	if (args.size() == 1 && !dice) {
		dice = std::unique_ptr<ex00::Dice>(new ex00::Dice);
		container->create_object(args[0], 
			std::unique_ptr<ex00::Dice_Service>(new ex00::Dice_Service(dice.get(), destroy_callback))
		);
	} else {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "need object path as argument");
	}
}


