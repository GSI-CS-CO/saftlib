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