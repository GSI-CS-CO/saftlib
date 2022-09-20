#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SoftwareActionSink_Proxy.hpp>
#include <SoftwareCondition_Proxy.hpp>

#include <saftbus/client.hpp>
#include <memory>
#include <iostream>

void on_action(uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed, uint16_t flags) {
	std::cerr << "event " << event << " " 
	          << "param " << param << " " 
	          << "deadline " << deadline.getTAI() << " "
	          << "executed " << executed.getTAI() << " " 
	          << "flags " << flags << " "
	          << std::endl;
}

int main(int argc, char **argv) {

	if (argc != 3 ) {
		std::cerr << "usage: " << argv[0] << " <name> <device>" << std::endl;
		return 1;
	}	


	auto saftd = eb_plugin::SAFTd_Proxy::create("/de/gsi/saftlib");

	auto tr_obj_path = saftd->AttachDevice(argv[1], argv[2]);
	for (auto &device: saftd->getDevices()) {
		std::cerr << device.first << " " << device.second << std::endl;
	}

	auto tr = eb_plugin::TimingReceiver_Proxy::create(tr_obj_path);
	auto sas_object_path = tr->NewSoftwareActionSink("");
	std::cerr << "sas_object_path = " << sas_object_path << std::endl;

	auto sas_proxy = eb_plugin::SoftwareActionSink_Proxy::create(sas_object_path);

	auto condition_obj_path = sas_proxy->NewCondition(true, 0, 0xffffffffffffffff, 0);
	std::cerr << "new Condition: " << condition_obj_path << std::endl; 

	auto cond_proxy = eb_plugin::SoftwareCondition_Proxy::create(condition_obj_path);
	cond_proxy->SigAction = &on_action;

	tr->InjectEvent(0,0,0);

	for (int i = 0; i < 5; ++i ) {
		saftbus::SignalGroup::get_global().wait_for_signal(1000);		
	}


	return 0;
}