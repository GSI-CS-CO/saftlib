
#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

extern "C" std::string get_object_path(saftbus::Container *container) {
	return std::string("/de/gsi/saftlib");
}

extern "C" std::unique_ptr<saftbus::Service> create_service(saftbus::Container *container) {

	std::unique_ptr<eb_plugin::SAFTd> instance(new eb_plugin::SAFTd(container, get_object_path(container)));
	std::unique_ptr<eb_plugin::SAFTd_Service> service(new eb_plugin::SAFTd_Service(std::move(instance)));
	return service;
}


