#ifndef TIMINGRECEIVER_PROXY_HPP_
#define TIMINGRECEIVER_PROXY_HPP_

#include "client.hpp"

#include <etherbone.h>

namespace saftbus {

	class TimingReceiver_Proxy : public Proxy {
	public:
		TimingReceiver_Proxy(const std::string &object_path, SignalGroup &signal_group);
		static std::shared_ptr<TimingReceiver_Proxy> create(const std::string &object_path, SignalGroup &signal_group = SignalGroup::get_global());
		bool signal_dispatch(int interface, Deserializer &signal_content);

		eb_data_t eb_read(eb_address_t adr);

	};

}

#endif
