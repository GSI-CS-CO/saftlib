#include "saftd.hpp"
#include "saftbus.hpp"
#include "server_connection.hpp"
#include "loop.hpp"
#include "make_unique.hpp"

#include <iostream>

namespace mini_saftlib {

	SAFTd_Proxy::SAFTd_Proxy(SignalGroup &signal_group)
		: Proxy::Proxy("/de/gsi/saftlib", signal_group)
	{	
	}

	std::shared_ptr<SAFTd_Proxy> SAFTd_Proxy::create(SignalGroup &signal_group) {
		return std::make_shared<SAFTd_Proxy>(signal_group);
	}

	void SAFTd_Proxy::greet(const std::string &name) {
		get_send().put(get_saftlib_object_id());
		int interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no = 0);
		get_send().put(name);
		get_connection().send(get_send());
	}
	bool SAFTd_Proxy::quit() {
		get_send().put(get_saftlib_object_id());
		int interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no = 1);
		get_connection().send(get_send());
		get_connection().receive(get_received());
		bool result;
		get_received().get(result);
		return result;
	}

	void SAFTd_Driver::greet(const std::string &name) {
		std::cout << "hello " << name << " from SAFTd" << std::endl;
	}

	bool SAFTd_Driver::quit() {
		std::cout << "shutting down server" << std::endl;
		return Loop::get_default().quit_in(std::chrono::milliseconds(1));
	}

	std::vector<std::string> SAFTd_Service::gen_interface_names() {
		std::vector<std::string> interface_names;
		interface_names.push_back("SAFTd");
		return interface_names;
	}

	SAFTd_Service::SAFTd_Service() 
		: Service(gen_interface_names())
	{
	}

	void SAFTd_Service::call(int interface_no, int function_no, Deserializer &received, Serializer &send)
	{
		switch(interface_no) {
			case 0:
			switch(function_no) {
				case 0:  {
					std::string name;
					received.get(name);
					impl->greet(name);
				}
				break;
				case 1: {
					bool result = impl->quit();
					send.put(result);
				}
				break;
				default:
					assert(false); // this is a severe error and can only happen if the code generator has a bug
			}
			break;
			default:
				assert(false); // this is a severe error and can only happen if the code generator has a bug
		}
	}



}