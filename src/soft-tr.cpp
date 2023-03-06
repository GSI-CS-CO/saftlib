#include <saftbus/loop.hpp>

#include "eb-source.hpp"

#include <etherbone.h>

#include <cstring>
#include <functional>

class Handler : public etherbone::Handler {
public:
	etherbone::Socket &socket;
	etherbone::Device device;
	Handler(etherbone::Socket &s) : socket(s) {
	}
	bool ping() {
		try {
			std::cerr << "ping" << std::endl;
			device.write(0x0, EB_DATAX, 0x12345678);
		} catch (etherbone::exception_t &e) {
			std::cerr << e << std::endl;
			saftbus::Loop::get_default().quit();
		}
		return false;
	}
	eb_status_t read(eb_address_t address, eb_width_t width, eb_data_t* data);
	eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
};

eb_status_t Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data) {
	std::cerr << "Handler::read()" << std::endl;
	*data = 12345;

	return EB_OK;
}

eb_status_t Handler::write(eb_address_t address, eb_width_t width, eb_data_t data) {
	std::cerr << "Handler::write()" << std::endl;
	if (address == 0xfffffff0) {
		try {
			std::cerr << "open 12346" << std::endl;
			device.open(socket, "tcp/localhost/12346");
			saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
				std::bind(&Handler::ping, this), std::chrono::milliseconds(0), std::chrono::milliseconds(1000) 
				);
		} catch (etherbone::exception_t &e) {
			std::cerr << e << std::endl;
		}
	} 
	if (address == 0xfffffff8) {
		try {
			std::cerr << "close 12346" << std::endl;
			device.close();
			saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
				[](){ saftbus::Loop::get_default().quit(); return false; }, std::chrono::milliseconds(0), std::chrono::milliseconds(1000) 
				);
		} catch (etherbone::exception_t &e) {
			std::cerr << e << std::endl;
		}
	} 
	return EB_OK;
}


int main(int argc, char *argv[]) {


	etherbone::Socket socket;
	socket.open("12345");


	etherbone::sdb_msi_device eb_slave_sdb;
	eb_slave_sdb.msi_first     = 0;
	eb_slave_sdb.msi_last      = 0xffff;
	eb_slave_sdb.abi_class     = 0;
	eb_slave_sdb.abi_ver_major = 0;
	eb_slave_sdb.abi_ver_minor = 0;
	eb_slave_sdb.bus_specific  = 0x8;//SDB_WISHBONE_WIDTH;
	eb_slave_sdb.sdb_component.addr_first = 0;
	eb_slave_sdb.sdb_component.addr_last  = UINT32_C(0xffffffff);
	eb_slave_sdb.sdb_component.product.vendor_id = 0x651;
	eb_slave_sdb.sdb_component.product.device_id = 0xefaa70;
	eb_slave_sdb.sdb_component.product.version   = 1;
	eb_slave_sdb.sdb_component.product.date      = 0x20150225;
	memcpy(eb_slave_sdb.sdb_component.product.name, "SAFTLIB           ", 19);


	Handler handler(socket);
	socket.attach(&eb_slave_sdb, &handler);
	
	saftbus::Loop::get_default().connect<saftlib::EB_Source>(socket);
	try {
		saftbus::Loop::get_default().run();
	} catch (etherbone::exception_t &e) {
		std::cerr << e << std::endl;

	}


	return 0;
}