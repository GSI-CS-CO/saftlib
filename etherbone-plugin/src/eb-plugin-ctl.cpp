#include "SAFTd_Proxy.hpp"

#include <saftbus/client.hpp>
#include <memory>
#include <iostream>

int main(int argc, char **argv) {

	auto saftd = eb_plugin::SAFTd_Proxy::create("/de/gsi/saftlib");

	if (argc == 3) {
		saftd->AttachDevice(argv[1], argv[2]);
	}

	if (argc == 2) {
		saftd->RemoveDevice(argv[1]);
	}

	auto devices = saftd->getDevices();

	for (auto &device: devices) {
		std::cerr << device.first << " -> " << device.second << std::endl;
	}

	return 0;
}