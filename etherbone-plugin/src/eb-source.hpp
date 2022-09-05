#ifndef TR_EB_SOURCE_HPP_
#define TR_EB_SOURCE_HPP_

#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#include <etherbone.h>

#include "eb-source.hpp"

#include <poll.h>

#include <loop.hpp>

#include <map>

namespace eb_plugin {

	class EB_Source : public saftbus::Source
	{
	public:
		EB_Source(etherbone::Socket socket_);

		static int add_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
		static int get_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);

		bool prepare(std::chrono::milliseconds &timeout_ms);
		bool check();
		bool dispatch();

		~EB_Source();
	private:
		etherbone::Socket socket;
		typedef std::map<int, struct pollfd> fd_map;
		fd_map fds;
	};

}

#endif
