#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>

int main(int argc, char *argv[]) {

	if (argc != 5 ) {
		std::cerr << "usage: " << argv[0] << " <name> <device> <name> <device>" << std::endl;
		return 1;
	}


	eb_plugin::SAFTd saftd;

	saftd.AttachDevice(argv[1], argv[2]);
	saftd.AttachDevice(argv[3], argv[4]);
	for (auto &device: saftd.getDevices()) {
		std::cerr << device.first << " " << device.second << std::endl;
	}
	eb_plugin::TimingReceiver* tr = saftd.getTimingReceiver(argv[1]);
	eb_plugin::TimingReceiver* tr2 = saftd.getTimingReceiver(argv[3]);


	for (int i = 0; i < 1; ++i) {
		auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
		std::cerr << "new NewSoftwareActionSink: " << softwareActionSink_obj_path << std::endl; 
		auto softwareActionSink = tr->getSoftwareActionSink(softwareActionSink_obj_path);


		auto condition_obj_path = softwareActionSink->NewCondition(true, 0, 0xffffffffffffffff, 0);
		std::cerr << "new Condition: " << condition_obj_path << std::endl; 


		tr->InjectEvent(0,0,0);
	}


	for (int i = 0; i < 1; ++i) {
		auto softwareActionSink_obj_path = tr2->NewSoftwareActionSink("");
		std::cerr << "new NewSoftwareActionSink: " << softwareActionSink_obj_path << std::endl; 
		auto softwareActionSink = tr2->getSoftwareActionSink(softwareActionSink_obj_path);


		auto condition_obj_path = softwareActionSink->NewCondition(true, 0, 0xffffffffffffffff, 0);
		std::cerr << "new Condition: " << condition_obj_path << std::endl; 


		tr2->InjectEvent(0,0,0);
	}


	for (int i = 0; i < 5 ; ++i) {
		saftbus::Loop::get_default().iteration(true);	
	}
	return 0;
}