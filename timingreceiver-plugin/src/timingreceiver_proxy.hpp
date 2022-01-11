#ifndef TIMINGRECEIVER_PROXY_HPP_
#define TIMINGRECEIVER_PROXY_HPP_

#include "client.hpp"

#include <etherbone.h>

namespace saftlib {

	class SAFTd_Proxy : public saftbus::Proxy {
	public:
		SAFTd_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group);
		static std::shared_ptr<SAFTd_Proxy> create(const std::string &object_path, saftbus::SignalGroup &signal_group = saftbus::SignalGroup::get_global());
		bool signal_dispatch(int interface, saftbus::Deserializer &signal_content);

		eb_data_t eb_read(eb_address_t adr);

	};

}

#endif
