#ifndef MINI_SAFTLIB_CLIENT_CONNECTION_
#define MINI_SAFTLIB_CLIENT_CONNECTION_

#include <saftbus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class ClientConnection {
		struct Impl; std::unique_ptr<Impl> d;		
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
	};


	class Proxy;

	class SignalGroup {
		struct Impl; std::unique_ptr<Impl> d;
	friend class Proxy;
	public:
		SignalGroup();
		~SignalGroup();

		int send_fd(Proxy &proxy);

		int wait_for_signal(int timeout_ms = -1);
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	};

	class Proxy {
		struct Impl; std::unique_ptr<Impl> d;
	friend class SignalGroup;
	public:
		Proxy(const std::string &object_path, SignalGroup &signal_group);
		virtual ~Proxy();
		virtual bool signal_dispatch(int interface, Deserializer &signal_content) {return true;};
		static std::shared_ptr<Proxy> create(SignalGroup &signal_group = SignalGroup::get_global());
		void quit();
	protected:
		static ClientConnection& get_connection();
		Serializer&              get_send();
		Deserializer&            get_received();
		int get_saftlib_object_id();
	};

}

#endif
