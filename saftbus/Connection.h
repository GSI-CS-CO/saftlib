#ifndef SAFTBUS_CONNECTION_H_
#define SAFTBUS_CONNECTION_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <giomm.h>

#include <map>
#include <set>

#include "Interface.h"
#include "saftbus.h"


namespace saftbus
{
	class Socket;

	struct ProxyPipe
	{
		int id;
		int fd;
		int socket_nr;
		bool operator<(const ProxyPipe& rhs) const {
			return id < rhs.id;
		}
	};

	// This class mimics the Gio::DBus::Connection class interface. It is limited to what is 
	// needed on the service side and all functionality for Proxy objects is removed.
	// This class is used by Saftlib to manage object lifetime of devices, receive remote function calls
	// and emit signals to Proxy objects.
	class Connection : public Glib::Object//Base
	{

	public:

		Connection(int number_of_sockets = 32, const std::string& base_name = "/tmp/saftbus_");

		guint 	register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable);
		bool 	unregister_object (guint registration_id);

		using SlotSignal = sigc::slot<void, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&>;

		// signal_subscribe and signal_unsubscribe are ONLY used by the driver of Owned.
		guint signal_subscribe 	( 	const SlotSignal&  	slot,
									const Glib::ustring&  	sender = Glib::ustring(),
									const Glib::ustring&  	interface_name = Glib::ustring(),
									const Glib::ustring&  	member = Glib::ustring(),
									const Glib::ustring&  	object_path = Glib::ustring(),
									const Glib::ustring&  	arg0 = Glib::ustring()//,
									//SignalFlags  	flags = SIGNAL_FLAGS_NONE 
			);
		void signal_unsubscribe 	( 	guint  	subscription_id	) ;

		void 	emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name=Glib::ustring(), const Glib::VariantContainerBase& parameters=Glib::VariantContainerBase());

		bool dispatch(Glib::IOCondition condition, Socket *socket);

	private:
		void handle_disconnect(Socket *socket);

		int socket_nr(Socket *socket);

		void print_all_fds();
		void clean_all_fds_from_socket(Socket *socket);

		void list_all_resources();

				// interface_name       // object_path
		std::map<Glib::ustring, std::map<Glib::ustring, int> > _saftbus_indices; 

		std::map<int, std::shared_ptr<InterfaceVTable> > _saftbus_objects;
		int _saftbus_object_id_counter; // log saftbus object creation
		int _saftbus_signal_handle_counter; // log signal subscriptions

		// TODO: use std::set instead of std::vector
		std::vector<std::shared_ptr<Socket> > _sockets; 

		//int _client_id;


		// 	     // handle    // signal
		std::map<guint, sigc::signal<void, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&> > _handle_to_signal_map;
		std::map<Glib::ustring, std::set<guint> > _id_handles_map;
		std::set<guint> _erased_handles;


		// store the pipes that go directly to one or many Proxy objects
				// interface_name        // object path
		std::map<Glib::ustring, std::map < Glib::ustring , std::set< ProxyPipe > > > _proxy_pipes;

		static int _saftbus_id_counter;

	};

}


#endif
