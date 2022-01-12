#include "SAFTd_proxy.hpp"

namespace saftlib {

	SAFTd_Proxy::SAFTd_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group)
		: Proxy(object_path, signal_group)
	{}

	std::shared_ptr<SAFTd_Proxy> SAFTd_Proxy::create(const std::string &object_path, saftbus::SignalGroup &signal_group)
	{
		return std::make_shared<SAFTd_Proxy>(object_path, signal_group);
	}
	bool SAFTd_Proxy::signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content)
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


	std::string SAFTd_Proxy::AttachDevice(const std::string& name, const std::string& path)
	{
		get_send().put(get_saftlib_object_id());
		unsigned interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no  = 1); 
		get_send().put(name); 
		get_send().put(path); 
		std::lock_guard<std::mutex> lock(get_client_socket());
		get_connection().send(get_send());
		get_connection().receive(get_received());
		std::string object_path;
		get_received().get(object_path);
		return object_path;		
	}
	void SAFTd_Proxy::RemoveDevice(const std::string& name)
	{
		get_send().put(get_saftlib_object_id());
		unsigned interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no  = 2); 
		get_send().put(name); 
		std::lock_guard<std::mutex> lock(get_client_socket());
		get_connection().send(get_send());
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