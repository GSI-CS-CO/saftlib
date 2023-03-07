
#include <memory>

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <TimingReceiverAddon.hpp>
#include "SimpleFirmware.hpp"

#include <saftbus/loop.hpp>




void on_MSI(uint32_t value) {
	std::cerr << "got MSI " << value << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <eb-device>" << std::endl;
		std::cerr << "   example: " << argv[0] << " dev/wbm0" << std::endl;
		return 1;
	} 

	try {

		saftlib::SAFTd saftd; 
		saftlib::TimingReceiver tr(saftd, "tr0", argv[1]);

		std::unique_ptr<saftlib::SimpleFirmware> simple_fw = std::unique_ptr<saftlib::SimpleFirmware>(new saftlib::SimpleFirmware(&saftd, &tr));

		simple_fw->MSI.connect(sigc::ptr_fun(on_MSI));
		simple_fw->start();

		for (;;) {
			saftbus::Loop::get_default().iteration(true);
		}

	} catch (std::runtime_error &e ) {
		std::cerr << "exception: " << e.what() << std::endl;
	}

	return 0;

}