#ifndef SAFTBUS_PROXY_CONNECTION_H_
#define SAFTBUS_PROXY_CONNECTION_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sigc++/sigc++.h>
//#include <giomm/asyncresult.h>

#include <thread>
#include <mutex>
#include <map>

#include "Interface.h"
#include "saftbus.h"
#include "core.h"


namespace saftbus
{

	class Proxy;

	// This class mimics the Gio::DBus::Connection object, but is limited to the functionality that is needed
	// on the Proxy side. The connection management on the Service side is done in the saftbus::Connection class.
	// This is different from the DBus interface where there is only one single Gio::DBus::Connection class. 
	// DBus is a many-to-many communication protocol, while saftbus is only a one-to-many protocoll. 
	class ProxyConnection /*: public Glib::Object*///Base
	{
	public:
		ProxyConnection(const std::string &base_name = socket_base_name);

		using SlotSignal = sigc::slot<void, const std::shared_ptr<ProxyConnection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Serial&>;

		// is used by Proxies to fetch properties
		Serial& call_sync(int saftbus_index,
			              const std::string& object_path, 
						  const std::string& interface_name, 
						  const std::string& method_name, 
						  const Serial& parameters, 
						  const std::string& bus_name=std::string(), 
						  int timeout_msec=-1);

	// internal stuff (not part the DBus fake api)
	public:
		void send_signal_flight_time(double signal_flight_time);
		void send_proxy_signal_fd(int pipe_fd, 
                                  std::string object_path,
                                  std::string interface_name,
                                  int global_id);
		void remove_proxy_signal_fd(int saftbus_index,
			                        std::string object_path,
                                    std::string interface_name,
                                    int global_id) ;

		std::string get_saftbus_id() { return _saftbus_id; }
		int get_connection_id();

		int get_saftbus_index(const std::string &object_path, const std::string &interface_name);

		// returned fd should only be used with a lock or in single threaded environments
		int get_fd() const {return _create_socket; }
	private:

		// this is the information that is needed to keep connected to a socket
		int _create_socket;
		struct sockaddr_un _address;
		std::string _filename;


		Serial _call_sync_result;
		//std::vector<char> _call_sync_result_buffer;

		Serial _result;


		std::string _saftbus_id; 

		int _connection_id;

		std::mutex _socket_mutex;
	};

}


#endif
