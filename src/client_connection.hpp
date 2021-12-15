#ifndef MINI_SAFTLIB_CLIENT_CONNECTION_
#define MINI_SAFTLIB_CLIENT_CONNECTION_

#include <saftbus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class ClientConnection {
	friend class SignalGroup;
	friend class Proxy;
	public:
		ClientConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ClientConnection();

		// send whatever data is in serial buffer to the server
		int send(Serializer &serdes, int timeout_ms = -1); 
		// wait for data to arrive from the server
		int receive(Deserializer &serdes, int timeout_ms = -1);

		void send_call();

	private:
		struct Impl; std::unique_ptr<Impl> d;		
	};


	class Proxy;

	class SignalGroup {
	public:
		SignalGroup();
		~SignalGroup();

		int send_fd(Proxy &proxy);

		int wait_for_signal(int timeout_ms = -1);
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	private:
		struct Impl; std::unique_ptr<Impl> d;
	};

	class Proxy {
	friend class SignalGroup;
	public:
		Proxy(const std::string &object_path, SignalGroup &signal_group);
		virtual ~Proxy();
		virtual bool signal_dispatch(int interface, Deserializer &signal_content) {return true;};
		static ClientConnection& get_connection();
	protected:
	private:
		struct Impl; std::unique_ptr<Impl> d;
	};

}

#endif
