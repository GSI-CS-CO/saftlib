
#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

#include <memory>
#include <vector>
#include <map>

std::unique_ptr<eb_plugin::SAFTd> saftd;
eb_plugin::SAFTd_Service *saftd_service; 
                                                                                    
extern "C" 
void destroy_service(saftbus::Service *service) {
	// this is a hint, that the Service object will no longer be needed an we can (if we want to) savely destroy the SAFTd object
	if (saftd_service && saftd_service == dynamic_cast<eb_plugin::SAFTd_Service*>(service)) {
		saftd.reset(); // destructor + release memory
		saftd_service = nullptr;
	}
}

extern "C" 
std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > create_services(saftbus::Container *container) {
	saftd         = std::move(std::unique_ptr<eb_plugin::SAFTd>(new eb_plugin::SAFTd(container)));
	saftd_service = new eb_plugin::SAFTd_Service(saftd.get(), &destroy_service);

	std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > services;
	services.push_back(std::make_pair(
		saftd->get_object_path(), 
		std::move(std::unique_ptr<eb_plugin::SAFTd_Service>(saftd_service))
		));

	return services;
}


