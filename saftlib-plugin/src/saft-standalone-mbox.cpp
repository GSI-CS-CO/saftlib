#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "SAFTd.hpp"
#include "Mailbox.hpp"
#include "OpenDevice.hpp"

#include <saftbus/loop.hpp>

#include <iostream>
#include <vector>
#include <exception>

struct LM32testbench : public saftlib::OpenDevice
                     , public saftlib::Mailbox 
{
	int msi_slot_to_this_program;

	LM32testbench(saftlib::SAFTd &saftd, const std::string &eb_path) 
		: OpenDevice(saftd.get_etherbone_socket(), eb_path, 10, &saftd)
		, Mailbox(OpenDevice::device)
	{
		eb_address_t msi_adr_to_this_program = saftd.request_irq(*this, std::bind(&LM32testbench::receiveMSI,this, std::placeholders::_1));
		msi_slot_to_this_program = ConfigureSlot(msi_adr_to_this_program);
		std::cerr << std::hex << "msi_adr of host: 0x" << msi_adr_to_this_program << std::dec << std::endl;
		std::cerr << "slot to host: " << msi_slot_to_this_program << std::endl;
	}
	~LM32testbench() {
		FreeSlot(msi_slot_to_this_program);
	}
	bool triggerMSI() {	
		static int cnt = 0;
		std::cerr << "triggerMSI: " << std::dec << cnt << std::endl;
		UseSlot(msi_slot_to_this_program, cnt++);
		return true;
	}
	void receiveMSI(eb_data_t data) { 
		std::cerr << "receiveMSI:     " << std::dec << data <<  std::endl;
	}

};

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <eb-device>" << std::endl;
		return 1;
	}
	saftlib::SAFTd saftd;

	try {

		LM32testbench testbench(saftd, argv[1]);
		// testbench.triggerMSI();

		saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(std::bind(&LM32testbench::triggerMSI,&testbench), std::chrono::milliseconds(1000));

		saftbus::Loop::get_default().run();
	} catch (std::runtime_error &e ) {
		std::cerr << "exception: " << e.what() << std::endl;
	}

	return 0;
}
