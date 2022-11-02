
#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <sstream>

std::unique_ptr<eb_plugin::SAFTd> saftd;
// eb_plugin::SAFTd_Service *saftd_service;
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
std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	if (!saftd) {
		// There can only be one saftd.
		// Always return the same instance of saftd.
		saftd         = std::move(std::unique_ptr<eb_plugin::SAFTd>(new eb_plugin::SAFTd(container)));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > services;
	// services.push_back(std::make_pair(
	// 	saftd->getObjectPath(), 
	// 	std::move(std::unique_ptr<eb_plugin::SAFTd_Service>(new eb_plugin::SAFTd_Service(saftd.get(), std::bind(&destroy_service))))
	// 	));

	for (auto &arg: args) {
		size_t pos = arg.find(':'); // the position of the first colon ':'
		if (pos == arg.npos || pos+1 == arg.size()) {
			throw std::runtime_error("expect <name>:<eb-path>[:<poll-interval>] as argument");
		}
		std::string name = arg.substr(0, pos);
		std::string path = arg.substr(pos+1);
		int poll_interval_ms;
		size_t pos2 = path.find(':'); // the position of the second colon ':'
		if (pos2 != path.npos) {
			if (pos2+1 == path.size()) { // 2nd colon is there, but poll inteval is missing
				throw std::runtime_error("expect <name>:<eb-path>[:<poll-interval>] as argument");
			} 
			std::istringstream poll_interval(path.substr(pos2+1));
			poll_interval >> poll_interval_ms;
			if (!poll_interval || poll_interval_ms <= 0) {
				throw std::runtime_error("cannot read poll interval. must be a positive integer");
			}
			path = path.substr(0,pos2);
		}
		saftd->AttachDevice(name, path, poll_interval_ms);
	}

	container->create_object(saftd->getObjectPath(), std::move(std::unique_ptr<eb_plugin::SAFTd_Service>(new eb_plugin::SAFTd_Service(saftd.get(), std::bind(&destroy_service)))));

	return services;
}


