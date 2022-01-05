#ifndef TIMINGRECEIVER_PROXY_HPP_
#define TIMINGRECEIVER_PROXY_HPP_

#include "client.hpp"

namespace mini_saftlib {

	class TimingReceiver_Proxy : public Proxy {
	public:
		TimingReceiver_Proxy(const std::string &object_path, SignalGroup &signal_group);
		static std::shared_ptr<TimingReceiver_Proxy> create(const std::string &object_path, SignalGroup &signal_group = SignalGroup::get_global());
		bool signal_dispatch(int interface, Deserializer &signal_content);
	};

}

#endif
