#include <string>
#include <iostream>

#include <Output.hpp>
#include <IoControl.hpp>

int main(int argc, char *argv[]) {
	if (argc != 2 && argc != 4) {
		std::cerr << "usage: " << argv[0] << " eb-path [ <io-name> <freq[Hz]> ]" << std::endl;
		return 0;
	}
	try {

		etherbone::Socket socket;
		socket.open();
		etherbone::Device device;
		device.open(socket, argv[1]);
		saftlib::IoControl io_control(device);
		// list all IOs
		if (argc == 2) {
			std::cout << "io-name                   " << std::endl;
			std::cout << "==========================" << std::endl;
			for (auto io: io_control.get_ios()) {
				std::cout << io.getName() << "   " << std::endl;
			}
			return 0;
		}

		// control the clock generator
		if (argc == 4) {
			std::istringstream fin(argv[3]);
			double freq;
			fin >> freq;
			if (!fin) {
				std::cerr << "cannot read frequency from argument " << argv[3] << std::endl;
				return 1;
			}

			// phase values are in nanoseconds
			double high_phase = 500000000/freq;
			double low_phase  = 500000000/freq;
			std::string io_name = argv[2];

			for (auto io: io_control.get_ios()) {
				// find the requested IO by name
				if (io.getName() == io_name) {
					io.WriteOutput(false);
					io.setGateOut(true);
					io.setOutputEnable(true);
					if (freq > 0) { // start the clk-gen
						std::cerr << "start clk-gen for " << io_name << " with " << freq << " Hz" << std::endl;
						io.StartClock(high_phase, low_phase, 0.0);
					}
					else { // stop the clk-gen
						std::cerr << "stop clk-gen for " << io_name << std::endl;
						io.StopClock();
						io.setOutputEnable(false);
					}
					return 0;
				}
			}

			std::cerr << "cannot find io-name " << io_name << std::endl;
			return 1;
		}

	} catch(etherbone::exception_t &e) {
		std::cerr << "Error: " << e << std::endl;
		return 1;
	}

	return 0;
}