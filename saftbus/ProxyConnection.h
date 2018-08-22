#ifndef SAFTBUS_PROXY_CONNECTION_H_
#define SAFTBUS_PROXY_CONNECTION_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <giomm.h>

#include <thread>
#include <map>

#include "Interface.h"
#include "saftbus.h"


namespace saftbus
{

	class Proxy;

	class ProxyConnection : public Glib::Object//Base
	{
	public:
		ProxyConnection(const Glib::ustring &base_name = "/tmp/saftbus_");

		// not needed by Proxies
		//guint 	register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info);
		// guint register_object(const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable);
		// bool  unregister_object(guint registration_id);

		using SlotSignal = sigc::slot<void, const Glib::RefPtr<ProxyConnection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&>;

		// is used by Proxies to fetch properties
		Glib::VariantContainerBase& call_sync (const Glib::ustring& object_path, 
											const Glib::ustring& interface_name, 
											const Glib::ustring& method_name, 
											const Glib::VariantContainerBase& parameters, 
											const Glib::ustring& bus_name=Glib::ustring(), 
											int timeout_msec=-1);


	// internal stuff (not part the DBus fake api)
	private:
		// bool dispatch(Glib::IOCondition condition);
		// void dispatchSignal();
	public:
		void register_proxy(Glib::ustring interface_name, Glib::ustring object_path, Proxy *proxy);

		// wait for response from server, expect a specific message type
		// if a signal is coming instead, handle the signal
		// do this until the correct type comes and return to caller
		bool expect_from_server(MessageTypeS2C type);

		int get_fd() const {return _create_socket; }
		Glib::ustring get_saftbus_id() { return _saftbus_id; }
		int get_connection_id();
	private:

		// this is the information that is needed to keep connected to a socket
		int _create_socket;
		struct sockaddr_un _address;
		std::string _filename;

		// having only one proxy per interface object path disallows multiple proxies of the same service in one process...
		//   .... maybe it does make sense to allow multiple proxies? TODO: take a decision !
		      // interface_name         object_path
		std::map<Glib::ustring, std::map<Glib::ustring, Proxy*> > _proxies; // maps object_paths to Proxies
		
		Glib::Variant<std::vector<Glib::VariantBase> > _call_sync_result;
		std::vector<char> _call_sync_result_buffer;

		Glib::VariantContainerBase _result;


		Glib::ustring _saftbus_id; 

		int _connection_id;

		std::mutex _socket_mutex;
	};

}


#endif
