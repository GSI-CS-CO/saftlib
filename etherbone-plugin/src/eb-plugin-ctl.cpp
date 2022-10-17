#include <src/SAFTd_Proxy.hpp>
#include <src/TimingReceiver_Proxy.hpp>
#include <src/SoftwareActionSink_Proxy.hpp>
#include <src/SoftwareCondition_Proxy.hpp>
#include <src/Output_Proxy.hpp>

#include <saftbus/error.hpp>
#include <saftbus/client.hpp>
#include <saftbus/loop.hpp>

#include <memory>
#include <iostream>

int action_count = 0;
std::shared_ptr<eb_plugin::TimingReceiver_Proxy> tr;
void on_action(uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed, uint16_t flags) {
	std::cerr << "event " << event << " " 
	          << "param " << param << " " 
	          << "deadline " << deadline.getTAI() << " "
	          << "executed " << executed.getTAI() << " " 
	          << "flags " << flags << " "
	          << std::endl;
	tr->InjectEvent(0,0,tr->CurrentTime()+1000000000); 
	std::cerr << "action_count=" << action_count << std::endl;
	++action_count;
}

int main(int argc, char *argv[]) {



	auto saftd = eb_plugin::SAFTd_Proxy::create("/de/gsi/saftlib");

	if (argc == 3 ) {

		std::string tr_obj_path;
		try {
			tr_obj_path = saftd->AttachDevice(argv[1], argv[2]);
		} catch (std::runtime_error &e) {
			std::cerr << e.what() << std::endl;
		}
		for (auto &device: saftd->getDevices()) {
			std::cerr << device.first << " " << device.second << std::endl;
		}

		tr = eb_plugin::TimingReceiver_Proxy::create("/de/gsi/saftlib/tr0");
		auto sas_object_path = tr->NewSoftwareActionSink("");
		std::cerr << "sas_object_path = " << sas_object_path << std::endl;
		// auto sas_object_path2 = tr->NewSoftwareActionSink("");
		// std::cerr << "sas_object_path2 = " << sas_object_path2 << std::endl;

		auto sas_proxy = eb_plugin::SoftwareActionSink_Proxy::create(sas_object_path);
		// auto sas_proxy2 = eb_plugin::SoftwareActionSink_Proxy::create(sas_object_path2);

		auto condition_obj_path = sas_proxy->NewCondition(true, 0, 0, 0);
		std::cerr << "new Condition: " << condition_obj_path << std::endl; 

		// auto condition_obj_path2 = sas_proxy2->NewCondition(true, 0, 0, 0);
		// std::cerr << "new Condition2: " << condition_obj_path2 << std::endl; 

		auto cond_proxy = eb_plugin::SoftwareCondition_Proxy::create(condition_obj_path);
		cond_proxy->SigAction = &on_action;


		// auto B1 = eb_plugin::Output_Proxy::create("/de/gsi/saftlib/tr0/outputs/B1");
		// B1->setOutputEnable(true);
		// B1->NewCondition(true, 0, 0xffffffffffffffff,         0, true);
		// B1->NewCondition(true, 0, 0xffffffffffffffff, 100000000, false);

		// auto B2 = eb_plugin::Output_Proxy::create("/de/gsi/saftlib/tr0/outputs/B2");
		// B2->setOutputEnable(true);
		// B2->NewCondition(true, 0, 0xffffffffffffffff,         0, true);
		// B2->NewCondition(true, 0, 0xffffffffffffffff, 100000000, false);


		// auto saftbus = saftbus::Container_Proxy::create();
		// saftbus->get_status();

		tr->InjectEvent(0,0,tr->CurrentTime()+1000000000); 

		for (;;) {
			std::cerr << "loop" << std::endl;
			saftbus::SignalGroup::get_global().wait_for_signal(2000);
			if (action_count > 4) {
				break;
			}
		}
	}


	if (argc == 2) {
		saftd->RemoveDevice(argv[1]);
	}

	return 0;
}
