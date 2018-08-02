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
	if (_debug_level) std::cerr << "Proxy::Proxy(" << name << "," << object_path << "," << interface_name << ") called   _connection_created = " << static_cast<bool>(_connection) << std::endl;

	if (!static_cast<bool>(_connection)) {
		if (_debug_level) std::cerr << "   this process has no ProxyConnection yet. Creating one now" << std::endl;
		_connection = Glib::RefPtr<saftbus::ProxyConnection>(new ProxyConnection);
		if (_debug_level) std::cerr << "   ProxyConnection created" << std::endl;
	}

	_connection->register_proxy(interface_name, object_path, this);
}


void Proxy::get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const 
{
	if (_debug_level) std::cerr << "Proxy::get_cached_property(" << property_name << ") called" << std::endl;

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
	if (_debug_level) std::cerr << "Proxy::on_properties_changed() called" << std::endl;
}
void Proxy::on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{
	if (_debug_level) std::cerr << "Proxy::on_signal() called" << std::endl;
}
Glib::RefPtr<saftbus::ProxyConnection> Proxy::get_connection() const
{
	if (_debug_level) std::cerr << "Proxy::get_connection() called " << std::endl;
	return _connection;
}

Glib::ustring Proxy::get_object_path() const
{
	if (_debug_level) std::cerr << "Proxy::get_object_path() called" << std::endl;
	return _object_path;
}
Glib::ustring Proxy::get_name() const
{
	if (_debug_level) std::cerr << "Proxy::get_name() called" << std::endl;
	return _name;
}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, const Glib::VariantContainerBase &query)
{
	std::cerr << "Proxy::call_sync(" << function_name << ") called" << std::endl;
	// call the Connection::call_sync in a special way that  it to cast the result in a special way. Otherwise the 
	// generated Proxy code cannot handle the resulting variant type.
	_result = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
			  	Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase> > >(
						_connection->call_sync(_object_path, 
		                			          _interface_name,
		                			          function_name,
		                			          query)).get_child(0));
	return _result;
}

}
