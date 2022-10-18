
#include "Dice.hpp"
#include "Dice_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<simple::Dice> dice;
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
std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > create_services(saftbus::Container *container) {
	if (!dice) {
		dice = std::move(std::unique_ptr<simple::Dice>(new simple::Dice()));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > services;
	services.push_back(std::make_pair(
		"/simple/Dice", 
		std::move(std::unique_ptr<simple::Dice_Service>(new simple::Dice_Service(dice.get(), std::bind(&destroy_service))))
		));

	return services;
}


