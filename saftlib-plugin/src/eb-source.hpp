#ifndef TR_EB_SOURCE_HPP_
#define TR_EB_SOURCE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <memory>
#include <vector>

#include <poll.h>

#include <saftbus/loop.hpp>

namespace saftlib {

	class EB_Source : public saftbus::Source
	{
	public:
		EB_Source(etherbone::Socket socket_);

		static int add_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
		static int get_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);

		bool prepare(std::chrono::milliseconds &timeout_ms);
		bool check();
		bool dispatch();
		std::string type(); 

		~EB_Source();
	private:
		etherbone::Socket socket;
		std::vector<pollfd> fds;
		std::vector<pollfd>::iterator fds_it;
		bool fds_it_valid;
	};

}

#endif
