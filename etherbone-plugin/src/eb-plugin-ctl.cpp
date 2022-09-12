#include "SAFTd_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"

#include <saftbus/client.hpp>
#include <memory>
#include <iostream>

int main(int argc, char **argv) {

	auto saftd = eb_plugin::SAFTd_Proxy::create("/de/gsi/saftlib");

	std::string path;
	if (argc == 3) {
		path = saftd->AttachDevice(argv[1], argv[2]);
	}

	if (argc == 2) {
		saftd->RemoveDevice(argv[1]);
	}

	auto devices = saftd->getDevices();

	for (auto &device: devices) {
		std::cerr << device.first << " -> " << device.second << std::endl;
	}

	auto tr = eb_plugin::TimingReceiver_Proxy::create(path);
	std::cerr << "locked: " << tr->getLocked() << std::endl;
	auto gateware_info = tr->getGatewareInfo();
	std::cerr << "gateware_info: " << std::endl;
	for(auto &pair: gateware_info) {
		std::cerr << pair.first << " " << pair.second << std::endl;
	}

	if (argc == 1) {
		saftd->Quit();
	}


	return 0;
}