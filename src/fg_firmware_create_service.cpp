#include "FunctionGeneratorFirmware.hpp"
#include "FunctionGeneratorFirmware_Service.hpp"

#include <SAFTd_Service.hpp>
#include <TimingReceiver_Service.hpp>

#include <saftbus/service.hpp>
#include <saftbus/error.hpp>

#include <memory>
#include <vector>
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


	for(auto &device: args) {
		std::cerr << "install function-generator firmware for " << device << std::endl;
		std::string device_object_path = object_path;
		device_object_path.append("/");
		device_object_path.append(device);
		saftlib::TimingReceiver_Service *tr_service = dynamic_cast<saftlib::TimingReceiver_Service*>(container->get_object(device_object_path));
		saftlib::TimingReceiver *tr = tr_service->d;


		std::unique_ptr<saftlib::FunctionGeneratorFirmware> fw(new saftlib::FunctionGeneratorFirmware(container, saftd, tr));
		saftlib::FunctionGeneratorFirmware *fw_ptr = fw.get();

		std::string addon_name = "FunctionGeneratorFirmware";
		tr->installAddon(addon_name, std::move(fw));
		if (container) {
			auto service = std::unique_ptr<saftlib::FunctionGeneratorFirmware_Service>(
						new saftlib::FunctionGeneratorFirmware_Service(
							fw_ptr, std::bind(&saftlib::TimingReceiver::removeAddon, tr, addon_name), false));
			std::cerr << "setting fw_ptr service to " << service.get() << std::endl;
			fw_ptr->set_service(service.get());
			container->create_object(fw_ptr->getObjectPath(), std::move(service));
		}


		try {
			fw_ptr->Scan(); // do the initial scan. If there's a exception here, the 
			                //  FunctionGeneratorFirmware driver should still be loaded 
			                //  that's why this is in a try catch block
		} catch (saftbus::Error &e) {
			std::cerr << "FunctionGeneratorFirmware::Scan failed because " << e.what() << std::endl;
		} catch (etherbone::exception_t &e) {
			throw saftbus::Error(saftbus::Error::FAILED, "etherbone exception");
		} catch (...) {
			throw saftbus::Error(saftbus::Error::FAILED, "unknown exception");
		}


	}

}


