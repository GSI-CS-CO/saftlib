#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>
#include <IoControl.hpp>
#include <Output.hpp>

#include <saftbus/client.hpp>
#include <memory>
#include <iostream>

int action_count = 0;

void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
	std::cout << "event " << event << " " 
	          << "param " << param << " " 
	          << "deadline " << deadline.getTAI() << " "
	          << "executed " << executed.getTAI() << " " 
	          << "flags " << flags << " "
	          << std::endl;
	++action_count;
}

int main(int argc, char *argv[]) {

	if (argc != 3 ) {
		std::cerr << "usage: " << argv[0] << " <name> <device>" << std::endl;
		return 1;
	}	


	saftlib::SAFTd saftd;

	std::cerr << argv[2] << std::endl;
	auto tr_obj_path = saftd.AttachDevice(argv[1], argv[2], 100);
	for (int n = 0; n < 2; ++n) {

		for (auto &device: saftd.getDevices()) {
			std::cerr << device.first << " " << device.second << std::endl;
		}

		saftlib::TimingReceiver* tr = saftd.getTimingReceiver(tr_obj_path);
		auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
		std::cerr << "new NewSoftwareActionSink: " << softwareActionSink_obj_path << std::endl; 

		auto softwareActionSink = tr->getSoftwareActionSink(softwareActionSink_obj_path);

		auto condition_obj_path = softwareActionSink->NewCondition(true, 0, 0xffffffffffffffff, 0);
		std::cerr << "new Condition: " << condition_obj_path << std::endl; 

		auto sw_condition = softwareActionSink->getCondition(condition_obj_path);
		sw_condition->Action = &on_action;

		auto B1 = tr->getOutput("/de/gsi/saftlib/tr0/outputs/B1");
		B1->setOutputEnable(true);
		B1->NewCondition(true, 0, 0xffffffffffffffff,         0, true);
		B1->NewCondition(true, 0, 0xffffffffffffffff, 100000000, false);

		auto B2 = tr->getOutput("/de/gsi/saftlib/tr0/outputs/B2");
		B2->setOutputEnable(true);
		B2->NewCondition(true, 0, 0xffffffffffffffff,         0, true);
		B2->NewCondition(true, 0, 0xffffffffffffffff, 100000000, false);



		auto timeout = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind([](saftlib::TimingReceiver* tr){
				std::cout << "inject event" << std::endl;
				tr->InjectEvent(0,0,tr->CurrentTime()+100000000); return true;
			}, tr), 
			std::chrono::milliseconds(1000));
		
		action_count = 0;
		for (;;) {
			saftbus::Loop::get_default().iteration(true);	
			if (action_count > 4) {
				break;
			}
		}

		saftbus::Loop::get_default().remove(timeout);

		tr->removeSowftwareActionSink(softwareActionSink);
	}
	return 0;
}
