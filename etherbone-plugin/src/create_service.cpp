#include "SAFTd_Service.hpp"
// #include "eb-source.hpp"
#include <saftbus/make_unique.hpp>
#include "SAFTd.hpp"
// #include <cstring>
// #include <saftbus/service.hpp>

// class MyHandler : public etherbone::Handler{
//   public:
//     ~MyHandler() { }

//     etherbone::status_t read (etherbone::address_t address, etherbone::width_t width, etherbone::data_t* data)
// 	{
// 		std::cout << "read_handler called\n";
// 		return EB_OK;
// 	}
//     etherbone::status_t write(etherbone::address_t address, etherbone::width_t width, etherbone::data_t  data)
// 	{
// 		static int i = 0;
// 		++i;
// 		std::cout << "write_handler called: adr=0x" << std::setfill('0') << std::setw(8) << std::hex << address 
// 		          << ", data=0x" << std::setfill('0') << std::setw(8) << std::hex << data << "\n";
// 		return EB_OK;
// 	}
// };

// MyHandler my_handler;
// struct sdb_device everything;

extern "C" std::string get_object_path(saftbus::Container *container) {
	return std::string("/de/gsi/saftlib");
}

extern "C" std::unique_ptr<saftbus::Service> create_service(saftbus::Container *container) {
	// etherbone::Socket socket;
	// etherbone::Device device;
	// saftbus::Container *container;

	// socket.open();
	// // device.open(socket, "dev/pts/5");

	// everything.abi_class                       = 0;//,
	// everything.abi_ver_major                   = 0;//,
	// everything.abi_ver_minor                   = 0;//,
	// everything.bus_specific                    = SDB_WISHBONE_WIDTH;//,
	// everything.sdb_component.addr_first        = 0;//,
	// everything.sdb_component.addr_last         = UINT32_C(0xffffffff);//,
	// everything.sdb_component.product.vendor_id = 0x651;//,
	// everything.sdb_component.product.device_id = 0xefaa70;//,
	// everything.sdb_component.product.version   = 1;//,
	// everything.sdb_component.product.date      = 0x20150225;//,
	// // strcpy((char*)everything.sdb_component.product.name, "SAFTLIB           ");

	// socket.attach(&everything, &my_handler);

	// saftbus::Loop::get_default().connect<eb_plugin::EB_Source>(socket);
	std::unique_ptr<eb_plugin::SAFTd> instance(new eb_plugin::SAFTd(container, get_object_path(container)));
	std::unique_ptr<eb_plugin::SAFTd_Service> service(new eb_plugin::SAFTd_Service(std::move(instance)));
	return service;
}


