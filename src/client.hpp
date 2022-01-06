#ifndef MINI_SAFTLIB_CLIENT_CONNECTION_
#define MINI_SAFTLIB_CLIENT_CONNECTION_

#include <saftbus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

namespace mini_saftlib {

	class ClientConnection {
		struct Impl; std::unique_ptr<Impl> d;		
	friend class SignalGroup;
	friend class Proxy;
	public:
		ClientConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ClientConnection();

		// send whatever data is in serial buffer to the server
		int send(Serializer &serializer, int timeout_ms = -1); 
		// wait for data to arrive from the server
		int receive(Deserializer &deserializer, int timeout_ms = -1);
	};


	class Proxy;

	class SignalGroup {
		struct Impl; std::unique_ptr<Impl> d;
	friend class Proxy;
	public:
		SignalGroup();
		~SignalGroup();

		int register_proxy(Proxy *proxy);
		void unregister_proxy(Proxy *proxy);

		int get_fd(); // this can be used to hook the SignalGroup into an event loop

		int wait_for_signal(int timeout_ms = -1);
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	};

	class Proxy {
		struct Impl; std::unique_ptr<Impl> d;
	friend class SignalGroup;
	public:
		virtual ~Proxy();
		virtual bool signal_dispatch(int interface, Deserializer &signal_content) = 0;
	protected:
		Proxy(const std::string &object_path, SignalGroup &signal_group);
		static ClientConnection& get_connection();
		Serializer&              get_send();
		Deserializer&            get_received();
		int                      get_saftlib_object_id();
		std::mutex&              get_client_socket();
	};

	class ContainerService_Proxy : public Proxy {
	public:
		ContainerService_Proxy(const std::string &object_path, SignalGroup &signal_group);
		static std::shared_ptr<ContainerService_Proxy> create(SignalGroup &signal_group = SignalGroup::get_global());
		bool signal_dispatch(int interface, Deserializer &signal_content);
		bool load_plugin(const std::string &so_filename, const std::string &object_path);
		void quit();
	};

}

#endif
