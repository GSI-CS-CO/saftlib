#include <saftbus/loop.hpp>

#include "eb-source.hpp"

#include <etherbone.h>

#include <cstring>
#include <iostream>
#include <iomanip>

class Handler : public etherbone::Handler {
public:
	sdb_device eb_slave_sdb;
	Handler(etherbone::Socket &socket) {
		eb_slave_sdb.abi_class     = 0;
		eb_slave_sdb.abi_ver_major = 0;
		eb_slave_sdb.abi_ver_minor = 0;
		eb_slave_sdb.bus_specific  = SDB_WISHBONE_WIDTH;
		eb_slave_sdb.sdb_component.addr_first = 0;
		eb_slave_sdb.sdb_component.addr_last  = UINT32_C(0xffffffff);
		eb_slave_sdb.sdb_component.product.vendor_id = 0x651;
		eb_slave_sdb.sdb_component.product.device_id = 0xefaa70;
		eb_slave_sdb.sdb_component.product.version   = 1;
		eb_slave_sdb.sdb_component.product.date      = 0x20150225;
		memcpy(eb_slave_sdb.sdb_component.product.name, "SAFTLIB           ", 19);
		
		socket.attach(&eb_slave_sdb, this);
	}
	
	eb_status_t read(eb_address_t address, eb_width_t width, eb_data_t* data);
	eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
};

eb_status_t Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data) {
	std::cerr << "Handler::read" << std::endl;
	*data = 0;
	return EB_OK;
}

eb_status_t Handler::write(eb_address_t address, eb_width_t width, eb_data_t data) {
	std::cerr << "Handler::write" << std::endl;
	return EB_OK;
}

bool quit() {
	saftbus::Loop::get_default().quit(); 
	return false;
}

etherbone::Socket socket;
etherbone::Device device;

int main(int argc, char *argv[]) {
	socket.open("12346");
	device.open(socket, "tcp/localhost/12345");

	Handler handler(socket);

	saftbus::Loop::get_default().connect<eb_plugin::EB_Source>(socket);

	saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
		[](){
			try { device.write(0xfffffff0, EB_DATAX, 0); } 
			catch (etherbone::exception_t &e) {std::cerr << e << std::endl;}
			return false;
		}, std::chrono::milliseconds(0), std::chrono::milliseconds(1000));
	
	saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
		[](){
			try { device.write(0xfffffff8, EB_DATAX, 0); } 
			catch (etherbone::exception_t &e) {std::cerr << e << std::endl;}
			return false;
		}, std::chrono::milliseconds(0), std::chrono::milliseconds(4000));
	
	saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
		[](){
			saftbus::Loop::get_default().quit();
			return false;
		}, std::chrono::milliseconds(0), std::chrono::milliseconds(5000));
	
	saftbus::Loop::get_default().run();

	device.close();
	socket.close();
	return 0;
}