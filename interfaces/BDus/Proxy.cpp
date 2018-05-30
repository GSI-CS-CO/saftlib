#include "Proxy.h"


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

}

void Proxy::reference()
{

}
void Proxy::unreference()
{

}

void Proxy::get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const 
{

}

void Proxy::on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< Glib::ustring >& invalidated_properties)
{

}
void Proxy::on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{

}
Glib::RefPtr<G10::BDus::Connection> Proxy::get_connection() const
{

}

Glib::ustring Proxy::get_object_path() const
{

}
Glib::ustring Proxy::get_name() const
{

}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, Glib::VariantContainerBase query)
{

}

}
}