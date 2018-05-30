#include "Connection.h"


namespace G10 
{

namespace BDus
{

void Connection::reference()
{

}
void Connection::unreference()
{

}

guint 	Connection::register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable)
{

}
bool 	Connection::unregister_object (guint registration_id)
{

}


guint Connection::signal_subscribe 	( 	const SlotSignal&  	slot,
										const Glib::ustring&  	sender,
										const Glib::ustring&  	interface_name,
										const Glib::ustring&  	member,
										const Glib::ustring&  	object_path,
										const Glib::ustring&  	arg0//,
										)//SignalFlags  	flags)
{
	return 0;
}

void Connection::signal_unsubscribe 	( 	guint  	subscription_id	) 
{
	
}


void 	Connection::emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
{

}


Glib::VariantContainerBase Connection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
{

}



}
}