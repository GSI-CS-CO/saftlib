
#include "DiceCup.hpp"
#include "DiceCup_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<ex02::DiceCup> dice_cup;
int ref_count = 0;                                                                                    

extern "C" 
void destroy_service() {
	--ref_count;
	std::cerr << "destroy_service was called " << ref_count << std::endl;
	// this is a hint, that the Service object will no longer be needed an we can (if we want to) savely destroy the SAFTd object
	if (ref_count == 0) {
		std::cerr << " destroying service" << std::endl;
		dice_cup.reset(); // destructor + release memory
	}
}

extern "C" 
std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > create_services(saftbus::Container *container) {
	std::string object_path = "/ex02/DiceCup";

	if (!dice_cup) {
		dice_cup = std::move(std::unique_ptr<ex02::DiceCup>(new ex02::DiceCup(object_path, container)));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > services;
	services.push_back(std::make_pair(
		"/ex02/DiceCup", 
		std::move(std::unique_ptr<ex02::DiceCup_Service>(new ex02::DiceCup_Service(dice_cup.get(), std::bind(&destroy_service))))
		));

	return services;
}


