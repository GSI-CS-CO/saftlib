#include "SAFTd_proxy.hpp"

namespace saftlib {

	SAFTd_Proxy::SAFTd_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group)
		: Proxy(object_path, signal_group)
	{}

	std::shared_ptr<SAFTd_Proxy> SAFTd_Proxy::create(const std::string &object_path, saftbus::SignalGroup &signal_group)
	{
		return std::make_shared<SAFTd_Proxy>(object_path, signal_group);
	}
	bool SAFTd_Proxy::signal_dispatch(int interface, saftbus::Deserializer &signal_content)
	{
		return true;
	}

	eb_data_t SAFTd_Proxy::eb_read(eb_address_t adr)
	{
		get_send().put(get_saftlib_object_id());
		unsigned interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no  = 0); 
		get_send().put(adr); 
		std::lock_guard<std::mutex> lock(get_client_socket());
		get_connection().send(get_send());
		get_connection().receive(get_received());
		eb_data_t dat;
		get_received().get(dat);
		return dat;
	}

	// void SAFTd_Proxy::quit() 
	// {
	// 	get_send().put(get_saftlib_object_id());
	// 	unsigned interface_no, function_no;
	// 	get_send().put(interface_no = 0);
	// 	get_send().put(function_no  = 2); 
	// 	std::lock_guard<std::mutex> lock(get_client_socket());
	// 	get_connection().send(get_send());
	// }
}