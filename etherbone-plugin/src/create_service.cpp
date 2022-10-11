
#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<eb_plugin::SAFTd> saftd;
eb_plugin::SAFTd_Service *saftd_service;
int ref_count = 0;
                                                                                    
extern "C" 
void destroy_service() {
	--ref_count;
	std::cerr << "destroy_service was called " << ref_count << std::endl;
	// this is a hint, that the Service object will no longer be needed an we can (if we want to) savely destroy the SAFTd object
	if (ref_count == 0) {
		std::cerr << " destroying service" << std::endl;
		saftd.reset(); // destructor + release memory
	}
}

extern "C" 
std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > create_services(saftbus::Container *container) {
	if (!saftd) {
		// There can only be one saftd.
		// Always return the same instance of saftd.
		saftd         = std::move(std::unique_ptr<eb_plugin::SAFTd>(new eb_plugin::SAFTd(container)));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > services;
	services.push_back(std::make_pair(
		saftd->get_object_path(), 
		std::move(std::unique_ptr<eb_plugin::SAFTd_Service>(new eb_plugin::SAFTd_Service(saftd.get(), std::bind(&destroy_service))))
		));

	return services;
}


