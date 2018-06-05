#include "Proxy.h"

#include <iostream>

namespace G10
{

namespace BDus
{

Proxy::Proxy(G10::BDus::BusType  	bus_type,
	const Glib::ustring&  	name,
	const Glib::ustring&  	object_path,
	const Glib::ustring&  	interface_name,
	const Glib::RefPtr< InterfaceInfo >&  	info,
	ProxyFlags  	flags
)
{
	std::cerr << "Proxy::Proxy(" << name << "," << object_path << "," << interface_name << ") called" << std::endl;

	// establish a connection to the service
	// ...
}

void Proxy::reference()
{

}
void Proxy::unreference()
{

}

void Proxy::get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const 
{
	std::cerr << "Proxy::get_cached_property(" << property_name << ") called" << std::endl;

	// fake a response
	if (property_name == "Devices")
	{
		std::map<Glib::ustring, Glib::ustring> devices;
		devices["tr0"] = "/de/gsi/saftlib";
		property = Glib::Variant< std::map< Glib::ustring, Glib::ustring > >::create(devices);
	}
}

void Proxy::on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< Glib::ustring >& invalidated_properties)
{
	std::cerr << "Proxy::on_properties_changed() called" << std::endl;
}
void Proxy::on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{
	std::cerr << "Proxy::on_signal() called" << std::endl;
}
Glib::RefPtr<G10::BDus::Connection> Proxy::get_connection() const
{
	std::cerr << "Proxy::get_connection() called " << std::endl;
}

Glib::ustring Proxy::get_object_path() const
{
	std::cerr << "Proxy::get_object_path() called" << std::endl;
}
Glib::ustring Proxy::get_name() const
{
	std::cerr << "Proxy::get_name() called" << std::endl;
}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, Glib::VariantContainerBase query)
{
	std::cerr << "Proxy::call_sync(" << function_name << ") called" << std::endl;
	std::vector<std::string> response_vector(1, "/de/gsi/saftlib/tr0/softwareActionSink");
	query = Glib::Variant< std::vector < std::string > >::create(response_vector);
	return query;
}

}
}