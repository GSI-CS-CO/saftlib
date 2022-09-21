#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>

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

int main(int argc, char *argv[]) {

	if (argc != 3 ) {
		std::cerr << "usage: " << argv[0] << " <name> <device>" << std::endl;
		return 1;
	}	


	eb_plugin::SAFTd saftd;

	auto tr_obj_path = saftd.AttachDevice(argv[1], argv[2]);
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
		std::bind([](eb_plugin::TimingReceiver* tr){tr->InjectEvent(0,0,0); return true;}, tr), 
		std::chrono::milliseconds(1000));

	for (int i = 0; i < 30 ; ++i) {
		saftbus::Loop::get_default().iteration(true);	
	}
	return 0;
}
