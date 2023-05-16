
#include "Dice.hpp"
#include "Dice_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<ex01::Dice> dice;

void destroy_dice() {
	if (dice) {
		dice->was_destroyed.emit();
		dice.reset(); // destructor + release memory
	}
}

extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	if (!dice) {
		dice = std::move(std::unique_ptr<ex01::Dice>(new ex01::Dice()));
		container->create_object("/ex01/Dice", std::move(std::unique_ptr<ex01::Dice_Service>(new ex01::Dice_Service(dice.get(), std::bind(&destroy_dice), false ))));
	}

}


