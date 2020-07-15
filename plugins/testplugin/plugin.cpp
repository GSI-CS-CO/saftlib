#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sigc++/sigc++.h>
#include <saftbus.h>
#include <MainContext.h>

#include "Driver1.h"

extern "C" {
	std::vector<std::string> get_interface_names()
	{
		std::vector<std::string> result;
		result.push_back("Driver1");
		result.push_back("Super");
		return result;
	}

	void *create_driver(const std::shared_ptr<Slib::MainContext> &context)
	{
		std::cerr << "creating a Driver1 instance" << std::endl;
		//context->iteration(false);
		// the driver plugin can attach sources to the context here
		return reinterpret_cast<void*>(new testplugin::Driver1(context));
	}

	void destroy_driver(void *driver)
	{
		std::cerr << "destroying a Driver1 instance" << std::endl;
		delete reinterpret_cast<testplugin::Driver1*>(driver);
	}

	void method_call(const std::string &method_name, const saftbus::Serial &parameters, saftbus::Serial &response)
	{
		std::cerr << "Driver1 method_call" << std::endl;
	}

	void property_get(const std::string &property_name, saftbus::Serial &value)
	{

	}

	void property_set(const std::string &property_name, const saftbus::Serial &value)
	{

	}

}
