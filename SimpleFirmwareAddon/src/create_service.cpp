#include "SimpleFirmware.hpp"
#include "SimpleFirmware_Service.hpp"

#include <SAFTd_Service.hpp>
#include <TimingReceiver_Service.hpp>

#include <saftbus/service.hpp>

#include <memory>
#include <vector>
#include <map>

std::map<std::string, std::unique_ptr<saftlib::SimpleFirmware> > simple_fw;

extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	std::cerr << "create_services args : ";
	for(auto &arg: args) {
		std::cerr << arg << " ";
	}
	std::cerr << std::endl;
	if (args.size() == 1) {
		std::string object_path = "/de/gsi/saftlib";
		saftlib::SAFTd_Service *saftd_service = dynamic_cast<saftlib::SAFTd_Service*>(container->get_object(object_path));
		saftlib::SAFTd *saftd = saftd_service->d;


		for(auto &device: args) {
			std::string device_object_path = object_path;
			device_object_path.append("/");
			device_object_path.append(device);
			saftlib::TimingReceiver_Service *tr_service = dynamic_cast<saftlib::TimingReceiver_Service*>(container->get_object(device_object_path));
			saftlib::TimingReceiver *tr = tr_service->d;


			std::unique_ptr<saftlib::SimpleFirmware> simple_fw(new saftlib::SimpleFirmware(saftd, tr));
			saftlib::SimpleFirmware *simple_fw_ptr = simple_fw.get();
			tr->installAddon("SimpleFirmware", std::move(simple_fw));

			container->create_object(simple_fw_ptr->getObjectPath(), std::move(std::unique_ptr<saftlib::SimpleFirmware_Service>(new saftlib::SimpleFirmware_Service(simple_fw_ptr))));
		}

	}
}


