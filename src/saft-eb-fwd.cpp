#include "interfaces/SAFTd.h"
#include <iostream>

int main(int argc, char *argv[])
{
	try {
		if (argc != 2) {
			std::cerr << "usage: " << argv[0] << " saftlib-device" << std::endl;
			std::cerr << std::endl;
			std::cerr << "example " << argv[0] << " tr0" << std::endl;
			return 1;
		}
    	std::shared_ptr<saftlib::SAFTd_Proxy> saftd = saftlib::SAFTd_Proxy::create();
    	std::cout << saftd->EbForward(std::string(argv[1])) << std::endl;
	} catch (saftbus::Error &e) {
		std::cerr << e.what() << std::endl;
		return 2;
	}
	return 0;
}