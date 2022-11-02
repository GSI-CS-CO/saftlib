
#include "Dice.hpp"
#include "Dice_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<ex01::Dice> dice;
int ref_count = 0;                                                                                    

extern "C" 
void destroy_service() {
	--ref_count;
	std::cerr << "destroy_service was called " << ref_count << std::endl;
	// this is a hint, that the Service object will no longer be needed an we can (if we want to) savely destroy the SAFTd object
	if (ref_count == 0) {
		std::cerr << " destroying service" << std::endl;
		dice.reset(); // destructor + release memory
	}
}

extern "C" 
void create_services(saftbus::Container *container) {
	if (!dice) {
		dice = std::move(std::unique_ptr<ex01::Dice>(new ex01::Dice()));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	container->create_object("/ex01/Dice", std::move(std::unique_ptr<ex01::Dice_Service>(new ex01::Dice_Service(dice.get(), std::bind(&destroy_service)))));
}


