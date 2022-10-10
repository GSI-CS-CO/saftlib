#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>
#include <IoControl.hpp>

#include <saftbus/client.hpp>
#include <memory>
#include <iostream>

int action_count = 0;

void on_action(uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed, uint16_t flags) {
	std::cerr << "event " << event << " " 
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


	eb_plugin::SAFTd saftd;

	std::cerr << argv[2] << std::endl;
	auto tr_obj_path = saftd.AttachDevice(argv[1], argv[2], 100);
	for (auto &device: saftd.getDevices()) {
		std::cerr << device.first << " " << device.second << std::endl;
	}

	eb_plugin::TimingReceiver* tr = saftd.getTimingReceiver(tr_obj_path);
	auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
	std::cerr << "new NewSoftwareActionSink: " << softwareActionSink_obj_path << std::endl; 

	auto softwareActionSink = tr->getSoftwareActionSink(softwareActionSink_obj_path);

	auto condition_obj_path = softwareActionSink->NewCondition(true, 0, 0xffffffffffffffff, 0);
	std::cerr << "new Condition: " << condition_obj_path << std::endl; 

	auto sw_condition = softwareActionSink->getCondition(condition_obj_path);
	sw_condition->SigAction = &on_action;



	saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
		std::bind([](eb_plugin::TimingReceiver* tr){
			std::cerr << "inject event" << std::endl;
			tr->InjectEvent(0,0,tr->CurrentTime()+100000000); return true;
		}, tr), 
		std::chrono::milliseconds(1000));

	for (;;) {
		saftbus::Loop::get_default().iteration(true);	
		if (action_count > 4) {
			break;
		}
	}
	return 0;
}
