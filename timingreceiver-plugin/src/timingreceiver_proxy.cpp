#include "timingreceiver_proxy.hpp"

namespace mini_saftlib {

	TimingReceiver_Proxy::TimingReceiver_Proxy(const std::string &object_path, SignalGroup &signal_group)
		: Proxy(object_path, signal_group)
	{}

	std::shared_ptr<TimingReceiver_Proxy> TimingReceiver_Proxy::create(const std::string &object_path, SignalGroup &signal_group)
	{
		return std::make_shared<TimingReceiver_Proxy>(object_path, signal_group);
	}
	bool TimingReceiver_Proxy::signal_dispatch(int interface, Deserializer &signal_content)
	{
		return true;
	}

	eb_data_t TimingReceiver_Proxy::eb_read(eb_address_t adr)
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

	// void TimingReceiver_Proxy::quit() 
	// {
	// 	get_send().put(get_saftlib_object_id());
	// 	unsigned interface_no, function_no;
	// 	get_send().put(interface_no = 0);
	// 	get_send().put(function_no  = 2); 
	// 	std::lock_guard<std::mutex> lock(get_client_socket());
	// 	get_connection().send(get_send());
	// }
}