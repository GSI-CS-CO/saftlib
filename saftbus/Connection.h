#ifndef SAFTBUS_CONNECTION_H_
#define SAFTBUS_CONNECTION_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sigc++/sigc++.h>

#include <map>
#include <set>

#include "Interface.h"
#include "saftbus.h"
#include "Logger.h"
#include "PollFD.h"
#include "core.h"


namespace saftbus
{
	class Socket;

	struct ProxyPipe
	{
		int id;
		int fd, fd_back;
		int socket_nr;
		bool operator<(const ProxyPipe& rhs) const {
			return id < rhs.id;
		}
	};

	// This class mimics the Gio::DBus::Connection class interface. It is limited to what is 
	// needed on the service side and all functionality for Proxy objects is removed.
	// This class is used by Saftlib to manage object lifetime of devices, receive remote function calls
	// and emit signals to Proxy objects.
	class Connection /*: public Glib::Object*/ //Base
	{

	public:

		Connection(int number_of_sockets = N_CONNECTIONS, const std::string& base_name = socket_base_name);
		~Connection();

		unsigned 	register_object (const std::string& object_path, const std::shared_ptr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable);
		bool 	unregister_object (unsigned registration_id);

		using SlotSignal = sigc::slot<void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Serial&>;

		// signal_subscribe and signal_unsubscribe are ONLY used by the driver of Owned.
		unsigned signal_subscribe 	( 	const SlotSignal&  	slot,
									const std::string&  	sender = std::string(),
									const std::string&  	interface_name = std::string(),
									const std::string&  	member = std::string(),
									const std::string&  	object_path = std::string(),
									const std::string&  	arg0 = std::string()//,
									//SignalFlags  	flags = SIGNAL_FLAGS_NONE 
			);
		void signal_unsubscribe 	( 	unsigned  	subscription_id	) ;

		void 	emit_signal (const std::string& object_path, 
			                 const std::string& interface_name, 
			                 const std::string& signal_name, 
			                 const std::string& destination_bus_name=std::string(), 
			                 const Serial& parameters=Serial());

		bool dispatch(Slib::IOCondition condition, Socket *socket);

	private:
		void handle_disconnect(Socket *socket);
		void proxy_pipe_garbage_collection();

		int socket_nr(Socket *socket);

		void print_all_fds();
		void clean_all_fds_from_socket(Socket *socket);

		void list_all_resources();

				// interface_name       // object_path
		std::map<std::string, std::map<std::string, int> > _saftbus_indices; 

		std::map<int, std::shared_ptr<InterfaceVTable> > _saftbus_objects;
		int _saftbus_object_id_counter; // log saftbus object creation
		int _saftbus_signal_handle_counter; // log signal subscriptions

		// TODO: use std::set instead of std::vector
		std::vector<std::shared_ptr<Socket> > _sockets; 

		//int _client_id;


		// 	     // handle    // signal
		std::map<unsigned, sigc::signal<void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Serial&> > _handle_to_signal_map;
		std::map<std::string, std::set<unsigned> > _id_handles_map;
		std::set<unsigned> _erased_handles;


		// store the pipes that go directly to one or many Proxy objects
				// interface_name        // object path
		std::map<std::string, std::map < std::string , std::set< ProxyPipe > > > _proxy_pipes;

		static int _saftbus_id_counter;

		std::map<int, std::string> _socket_owner;

		// histograms for timing analysis
		std::map<int, int> _signal_flight_times;

		std::map<std::string, std::map<int, int> > _function_run_times;

		Logger logger;

		bool _create_signal_flight_time_statistics;
	};

}


#endif
