#include <memory>

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include "SimpleFirmware.hpp"

#include <saftbus/loop.hpp>

void on_MSI(uint32_t value) {
	std::cerr << "got MSI " << value << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <eb-device>" << std::endl;
		std::cerr << "   example: " << argv[0] << " dev/wbm0 " << std::endl;
		return 1;
	}

	try {

		saftlib::SAFTd saftd;
		saftlib::TimingReceiver tr(saftd, "tr0", argv[1]);

		SimpleFirmware simple_fw(&tr);
		tr.installAddon("SimpleFirmware", &simple_fw);

		auto interfaces = tr.getInterfaces();
		// for (auto &interface: interfaces) {
		// 	std::cout << "Interface: " << interface.first << std::endl;
		// 	for (auto &name_objpath: interface.second) {
		// 		std::cout << "   " << std::setw(20) << name_objpath.first << " " << name_objpath.second << std::endl;
		// 	}
		// }

		simple_fw.MSI.connect(sigc::ptr_fun(on_MSI));
		simple_fw.start();

		saftbus::Loop::get_default().run();


	} catch (std::runtime_error &e ) {
		std::cerr << "exception: " << e.what() << std::endl;
	}

	return 0;
}