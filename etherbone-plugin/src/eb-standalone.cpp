#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>

int main(int argc, char *argv[]) {

	if (argc != 3 ) {
		std::cerr << "usage: " << argv[0] << " <name> <device>" << std::endl;
		return 1;
	}


	eb_plugin::SAFTd saftd;

	saftd.AttachDevice(argv[1], argv[2]);
	for (auto &device: saftd.getDevices()) {
		std::cerr << device.first << " " << device.second << std::endl;
	}
	eb_plugin::TimingReceiver* tr = saftd.getTimingReceiver(argv[1]);


	for (int i = 0; i < 22; ++i) {
		auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
		std::cerr << "new NewSoftwareActionSink: " << softwareActionSink_obj_path << std::endl; 
		auto softwareActionSink = tr->getSoftwareActionSink(softwareActionSink_obj_path);


		auto condition_obj_path = softwareActionSink->NewCondition(true, i, 0xffffffffffffffff, 0);
		std::cerr << "new Condition: " << condition_obj_path << std::endl; 


		tr->InjectEvent(0,0,0);
	}


	for (int i = 0; i < 5 ; ++i) {
		saftbus::Loop::get_default().iteration(true);	
	}
	return 0;
}