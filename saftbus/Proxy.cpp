#include "Proxy.h"

#include <iostream>

#include "saftbus.h"
#include "core.h"

namespace saftbus
{

Glib::RefPtr<saftbus::ProxyConnection> Proxy::_connection;
bool Proxy::_connection_created = false;

Proxy::Proxy(saftbus::BusType  	bus_type,
	const Glib::ustring&  	name,
	const Glib::ustring&  	object_path,
	const Glib::ustring&  	interface_name,
	const Glib::RefPtr< InterfaceInfo >&  	info,
	ProxyFlags  	flags
) : _name(name)
  , _object_path(object_path)
  , _interface_name(interface_name)
{
	std::cerr << "Proxy::Proxy(" << name << "," << object_path << "," << interface_name << ") called   _connection_created = " << _connection.get() << std::endl;

	if (!_connection.get()) {
		std::cerr << "   this process has no ProxyConnection yet. Creating one now" << std::endl;
		_connection = Glib::RefPtr<saftbus::ProxyConnection>(new ProxyConnection);
		std::cerr << "   ProxyConnection created" << std::endl;
	}

	// establish a connection to the service: the service needs to know which proxies are connected in order to dispatch the incoming signals
	// ...
	//_connection->register_proxy(_interface_name, _object_path, this);
}


void Proxy::get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const 
{
	std::cerr << "Proxy::get_cached_property(" << property_name << ") called" << std::endl;

	return; // empty response

	// // fake a response
	// if (property_name == "Devices")
	// {
	// 	std::map<Glib::ustring, Glib::ustring> devices;
	// 	devices["tr0"] = "/de/gsi/saftlib";
	// 	property = Glib::Variant< std::map< Glib::ustring, Glib::ustring > >::create(devices);
	// }
}

void Proxy::on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< Glib::ustring >& invalidated_properties)
{
	std::cerr << "Proxy::on_properties_changed() called" << std::endl;
}
void Proxy::on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{
	std::cerr << "Proxy::on_signal() called" << std::endl;
}
Glib::RefPtr<saftbus::ProxyConnection> Proxy::get_connection() const
{
	std::cerr << "Proxy::get_connection() called " << std::endl;
	return _connection;
}

Glib::ustring Proxy::get_object_path() const
{
	std::cerr << "Proxy::get_object_path() called" << std::endl;
	return _object_path;
}
Glib::ustring Proxy::get_name() const
{
	std::cerr << "Proxy::get_name() called" << std::endl;
	return _name;
}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, const Glib::VariantContainerBase &query)
{
	// std::cerr << "Proxy::call_sync(" << function_name << ") called" << std::endl;
	// Glib::VariantContainerBase &result = _connection->call_sync(_object_path, 
	// 	                          _interface_name,
	// 	                          function_name,
	// 	                          query);
	// std::cerr << "result = " << result.print() << std::endl;
	// return result;

	// I'm duplicating large parts of the ProxyConnection::call_sync because of small differences in how method calls and property get/set are handled

	std::cerr << "Proxy::call_sync(" << _object_path << "," << _interface_name << "," << function_name << ") called" << std::endl;
	// first append message meta informations like: type of message, recipient, sender, interface name
	std::vector<Glib::VariantBase> message;
	message.push_back(Glib::Variant<Glib::ustring>::create(_object_path));
	message.push_back(Glib::Variant<Glib::ustring>::create(_connection->get_saftbus_id()));
	message.push_back(Glib::Variant<Glib::ustring>::create(_interface_name));
	message.push_back(Glib::Variant<Glib::ustring>::create(function_name)); // name can be method_name or property_name
	message.push_back(query);
	// then convert inot a variant vector type
	Glib::Variant<std::vector<Glib::VariantBase> > var_message = Glib::Variant<std::vector<Glib::VariantBase> >::create(message);
	// serialize
	guint32 size = var_message.get_size();
	const char *data_ptr =  static_cast<const char*>(var_message.get_data());
	// send over socket
	saftbus::write(_connection->get_fd(), saftbus::METHOD_CALL);
	saftbus::write(_connection->get_fd(), size);
	saftbus::write_all(_connection->get_fd(), data_ptr, size);

	// receive from socket
	saftbus::MessageTypeS2C type;
	saftbus::read(_connection->get_fd(), type);
	std::cerr << "got response " << type << std::endl;
	saftbus::read(_connection->get_fd(), size);
	_call_sync_result_buffer.resize(size);
	saftbus::read_all(_connection->get_fd(), &_call_sync_result_buffer[0], size);

	// deserialize
	//Glib::Variant<std::vector<Glib::VariantBase> > payload;
	deserialize(_call_sync_result, &_call_sync_result_buffer[0], _call_sync_result_buffer.size());

	std::cerr << "Proxy::call_sync _call_sync_result = " << _call_sync_result.get_type_string() << " " << _call_sync_result.print() << std::endl;

	_result = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(_call_sync_result.get_child(0));

	std::cerr << "Proxy::call_sync _result = " << _result.get_type_string() << " " << _result.print() << std::endl;
	//Glib::VariantBase result    = Glib::VariantBase::cast_dynamic<Glib::VariantBase >(payload.get_child(0));
	// std::cerr << " payload.get_type_string() = " << _call_sync_result.get_type_string() << "     .value = " << _call_sync_result.print() << std::endl;
	// for (unsigned n = 0; n < _call_sync_result.get_n_children(); ++n)
	// {
	// 	Glib::VariantBase child = _call_sync_result.get_child(n);
	// 	std::cerr << "     parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
	// }
	// std::cerr << "just before returning " << _call_sync_result.print() << std::endl;
	return _result;		

}

}
