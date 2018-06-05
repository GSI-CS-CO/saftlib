#include "Connection.h"

#include <iostream>

namespace G10 
{

namespace BDus
{

Connection::Connection()
	: _bdus_objects()
	, counter(0)
{
}

void Connection::reference()
{

}
void Connection::unreference()
{

}

guint Connection::register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable)
{
	std::cerr << "Connection::register_object("<< object_path <<") called" << std::endl;
	//_bdus_objects[object_path] = counter++;
	guint result = _bdus_objects.size();
	_bdus_objects.push_back(object_path);
	for(auto iter = _bdus_objects.begin(); iter != _bdus_objects.end(); ++iter)
	{
		std::cerr << *iter << std::endl;
	}
	return result;
}
bool Connection::unregister_object (guint registration_id)
{
	if (registration_id < _bdus_objects.size())
	{
		_bdus_objects[registration_id] = "";
		return true;
	}
	return false;
}


guint Connection::signal_subscribe 	( 	const SlotSignal&  	slot,
										const Glib::ustring&  	sender,
										const Glib::ustring&  	interface_name,
										const Glib::ustring&  	member,
										const Glib::ustring&  	object_path,
										const Glib::ustring&  	arg0//,
										)//SignalFlags  	flags)
{
	std::cerr << "Connection::signal_subscribe() called" << std::endl;
	return 0;
}

void Connection::signal_unsubscribe 	( 	guint  	subscription_id	) 
{
	std::cerr << "Connection::signal_unsubscribe() called" << std::endl;
}


void 	Connection::emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
{
	std::cerr << "Connection::emit_signal() called" << std::endl;
}


Glib::VariantContainerBase Connection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
{
	std::cerr << "Connection::call_sync(" << object_path << "," << interface_name << "," << method_name << ") called" << std::endl;
	return Glib::VariantContainerBase();
}



}
}