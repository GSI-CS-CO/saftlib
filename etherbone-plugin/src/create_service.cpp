#include "Device_Service.hpp"
#include "eb-source.hpp"
#include <saftbus/make_unique.hpp>

extern "C" std::unique_ptr<saftbus::Service> create_service() {
	etherbone::Socket socket;
	socket.open();
	saftbus::Loop::get_default().connect<eb_plugin::EB_Source>(socket);

	return std2::make_unique<eb_plugin::Device_Service>();
}
