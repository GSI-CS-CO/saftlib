#include "BurstGenerator.hpp"
#include "BurstGenerator_Service.hpp"

#include <SAFTd_Service.hpp>
#include <SAFTd.hpp>
#include <TimingReceiver_Service.hpp>

#include <saftbus/service.hpp>

#include <memory>
#include <vector>
#include <sstream>
#include <map>
#include <functional>


extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	std::cerr << "create_services args : ";
	for(auto &arg: args) {
		std::cerr << arg << " ";
	}
	std::cerr << std::endl;
	std::string object_path = "/de/gsi/saftlib";
	saftlib::SAFTd_Service *saftd_service = dynamic_cast<saftlib::SAFTd_Service*>(container->get_object(object_path));
	saftlib::SAFTd *saftd = saftd_service->d;
	auto all_devices = saftd->getDevices();

	for (unsigned i = 0; i < args.size(); ++i) {
		const std::string &device = args[i];
		if (all_devices.find(device) == all_devices.end()) {
			std::cerr << "cannot find device with name " << device << std::endl;
			return;
		}
		std::cerr << "install burst-generator firmware for " << device << std::endl;

		std::string device_object_path = object_path;
		device_object_path.append("/");
		device_object_path.append(device);
		saftlib::TimingReceiver_Service *tr_service = dynamic_cast<saftlib::TimingReceiver_Service*>(container->get_object(device_object_path));
		saftlib::TimingReceiver *tr = tr_service->d;

		// check if firmware binary needs to be programmed 
		if ((i+1) < args.size() && all_devices.find(args[i+1]) == all_devices.end()) {
			int cpu_idx = -1; // -1 means not to load the firmware binary
			                  // integer >=0 means load the firmware binary into this LM32 core 
			std::istringstream in(args[i+1]);
			in >> cpu_idx;
			if (!in) {
				std::cerr << "cannot read cpu index from argument " << args[i+1] << std::endl;
				return;
			}
			if (cpu_idx >= 0 && cpu_idx >= (int)tr->LM32Cluster::getCpuCount()) {
				std::cerr << "Invalid cpu index " << cpu_idx << " , hardware has only " << tr->LM32Cluster::getCpuCount() << " LM32 cores. " << std::endl;
				return;
			}
			if (cpu_idx >= 0) {	// stop the cpu, write firmware and reset cpu
				tr->SafeHaltCpu(cpu_idx);
				std::cerr << "writing firmware " << DATADIR "/firmware/burstgen.bin to cpu[" << cpu_idx << "]" << std::endl;
				std::string firmware_bin(DATADIR "/firmware/burstgen.bin");
				tr->WriteFirmware(cpu_idx, firmware_bin);
				tr->CpuReset(cpu_idx);
			}
			++i;
		}


		std::string addon_name = "BurstGenerator";
		std::unique_ptr<saftlib::BurstGenerator> burstgenerator_fw(new saftlib::BurstGenerator(container, saftd, tr));
		std::unique_ptr<saftlib::BurstGenerator_Service> service(new saftlib::BurstGenerator_Service(burstgenerator_fw.get(), std::bind(&saftlib::TimingReceiver::removeAddon, tr, addon_name)));
		burstgenerator_fw->set_service(service.get());
		std::string object_path = burstgenerator_fw->getObjectPath();
		tr->installAddon(addon_name, std::move(burstgenerator_fw));
		container->create_object(object_path, std::move(service));
	}

}


