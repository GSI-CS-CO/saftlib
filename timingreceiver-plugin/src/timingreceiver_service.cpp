#include "timingreceiver_service.hpp"

#include <loop.hpp>



#include <iostream>

namespace timingreceiver 
{

	class EB_Source : public mini_saftlib::Source
	{
		public:
			EB_Source(etherbone::Socket socket_);
		protected:
			
			static int add_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
			static int get_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
		
			bool prepare(std::chrono::milliseconds& timeout_ms) override;
			bool check() override;
			bool dispatch() override;
			
		private:
			etherbone::Socket socket;
			
			typedef std::map<int, struct pollfd> fd_map;
			fd_map fds;
	};


	EB_Source::EB_Source(etherbone::Socket socket_)
	 : socket(socket_)
	{}

	int EB_Source::add_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		//std::cerr << "EB_Source::add_fd(" << fd << ")" << std::endl;
		EB_Source* self = (EB_Source*)data;
		
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLERR;

		std::pair<fd_map::iterator, bool> res = 
			self->fds.insert(fd_map::value_type(fd, pfd));
		
		if (res.second) // new element; add to poll
			self->add_poll(res.first->second);
		
		int flags = POLLERR | POLLHUP;
		if ((mode & EB_DESCRIPTOR_IN)  != 0) flags |= POLLIN;
		if ((mode & EB_DESCRIPTOR_OUT) != 0) flags |= POLLOUT;
		
		res.first->second.events |= flags;
		return 0;
	}

	int EB_Source::get_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		EB_Source* self = (EB_Source*)data;
		
		fd_map::iterator i = self->fds.find(fd);
		if (i == self->fds.end()) return 0;
		
		int flags = i->second.revents;
		
		return 
			((mode & EB_DESCRIPTOR_IN)  != 0 && (flags & (POLLIN  | POLLERR | POLLHUP)) != 0) ||
			((mode & EB_DESCRIPTOR_OUT) != 0 && (flags & (POLLOUT | POLLERR | POLLHUP)) != 0);
	}

	static int no_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		return 0;
	}

	bool EB_Source::prepare(std::chrono::milliseconds& timeout_ms)
	{
		std::cerr << "EB_Source::prepare" << std::endl;
		// Retrieve cached current time
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		
		// Work-around for no TX flow control: flush data now
		socket.check(now_sec, 0, &no_fd);
		
		// Clear the old requested FD status
		for (fd_map::iterator i = fds.begin(); i != fds.end(); ++i)
			i->second.events = POLLERR;
		
		// Find descriptors we need to watch
		socket.descriptors(this, &EB_Source::add_fd);
		
		// Eliminate any descriptors no one cares about any more
		fd_map::iterator i = fds.begin();
		while (i != fds.end()) {
			fd_map::iterator j = i;
			++j;
			if ((i->second.events & POLLHUP) == 0) {
				remove_poll(i->second);
				fds.erase(i);
			}
			i = j;
		}
		
		// Determine timeout
		uint32_t timeout = socket.timeout();
		if (timeout) {
			timeout_ms = std::chrono::milliseconds(static_cast<int64_t>(timeout)*1000 - now_sec);
			if (timeout_ms < std::chrono::milliseconds(0)) timeout_ms = std::chrono::milliseconds(0);
		} else {
			timeout_ms = std::chrono::milliseconds(-1);
		}
		
		return (timeout_ms == std::chrono::milliseconds(0)); // true means immediately ready
	}

	bool EB_Source::check()
	{
		std::cerr << "EB_Source::check" << std::endl;
		bool ready = false;
		
		// Descriptors ready?
		for (fd_map::const_iterator i = fds.begin(); i != fds.end(); ++i)
			ready |= (i->second.revents & i->second.events) != 0;
		
		// Timeout ready?
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		ready |= socket.timeout() >= now_sec;
		
		return ready;
	}

	bool EB_Source::dispatch()
	{
		 std::cerr << "EB_Source::dispatch" << std::endl;
		// Process any pending packets
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		socket.check(now_sec, this, &EB_Source::get_fd);
		
		// Run the no-op signal
		return true;
	}



	bool timeout() {
		static int i = 0;
		std::cerr << "timeout #" << i++ << std::endl;
		// auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
		// mini_saftlib::Loop::get_default().quit();
		if (i == 5) {
			return false;
		}
		return true;
	}

	Timingreceiver_Serivce::Timingreceiver_Serivce()
		: mini_saftlib::Service(gen_interface_names())
	{
		std::cerr << "connect timeout source" << std::endl;
		mini_saftlib::Loop::get_default().connect(
			std::move(
				std2::make_unique<mini_saftlib::TimeoutSource>(
					sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
				) 
			)
		);

		std::cerr << "Timingreceiver_Serivce constructor socket.open()" << std::endl;
		socket.open();
		std::cerr << "Timingreceiver_Serivce constructor attach eb_source" << std::endl;
		mini_saftlib::Loop::get_default().connect(std::move(std2::make_unique<EB_Source>(socket)));
		std::cerr << "Timingreceiver_Serivce constructor done" << std::endl;
	}

	std::vector<std::string> Timingreceiver_Serivce::gen_interface_names()
	{
		std::vector<std::string> result;
		result.push_back("de.gsi.saftlib.TimingReceiver");
		return result;
	}

	void Timingreceiver_Serivce::call(unsigned interface_no, unsigned function_no, int client_fd, mini_saftlib::Deserializer &received, mini_saftlib::Serializer &send)
	{

	}

}

extern "C" std::unique_ptr<mini_saftlib::Service> create_service() {
	return std2::make_unique<timingreceiver::Timingreceiver_Serivce>();
}
