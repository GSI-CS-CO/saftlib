#ifndef SAFTBUS_CLIENT_CONNECTION_HPP_
#define SAFTBUS_CLIENT_CONNECTION_HPP_

#include "saftbus.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

#include <unistd.h>

namespace saftbus {

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

		// @saftbus-export
		int wait_for_signal(int timeout_ms = -1);
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	};

	class Proxy {
		struct Impl; std::unique_ptr<Impl> d;
	friend class SignalGroup;
	public:
		virtual ~Proxy();
		virtual bool signal_dispatch(int interface_no, int signal_no, Deserializer &signal_content) = 0;
	protected:
		// The Proxy constructor connects the Proxy with a given Service object (identified by the object_path)
		// and connencts the SignalGroup for it.
		// interfaces_names is an array of strings with the interface names in text from.
		// During initialization, the Proxy asks the Service object for the indices that correspont to 
		// the given interface names on this particular service object. The mapping is provided to all 
		// base classes by the method interface_no_from_name.
		Proxy(const std::string &object_path, SignalGroup &signal_group, const std::vector<std::string> &interface_names);
		static ClientConnection& get_connection();
		Serializer&              get_send();
		Deserializer&            get_received();
		int                      get_saftlib_object_id();
		std::mutex&              get_client_socket();


		// this needs to be called by derived classes in order to determine
		// which interface_no they refer to.
		// A specific Proxy only knows the interface name, but not under which number this name can be addressed
		// in the Service object. The Proxy constructor has to get this name->number mapping from the
		// Service object during the initialization phase (the Proxy constructor)
		int interface_no_from_name(const std::string &interface_name); 
	};

	struct SaftbusInfo {
		struct ObjectInfo {
			unsigned object_id;
			std::string object_path;
			std::vector<std::string> interface_names;
			std::map<int, int> signal_fds_use_count;
			int owner;
		};
		std::vector<ObjectInfo> object_infos;
		struct ClientInfo {
			pid_t process_id;
			int client_fd;
			struct SignalFD {
				int fd;
				int use_count;
			};
			std::map<int,int> signal_fds;
		};
		std::vector<ClientInfo> client_infos;
	};
	class Container_Proxy : public Proxy {
	public:
		Container_Proxy(const std::string &object_path, SignalGroup &signal_group, std::vector<std::string> interface_names);
		static std::shared_ptr<Container_Proxy> create(SignalGroup &signal_group = SignalGroup::get_global());
		// @saftbus-export
		bool signal_dispatch(int interface_no, 
			                 int signal_no, 
			                 Deserializer& signal_content);
		bool load_plugin(const std::string &so_filename);
		bool remove_object(const std::string &object_path);
		void quit();
		SaftbusInfo get_status();
	};

}

#endif
