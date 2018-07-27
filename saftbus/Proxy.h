#ifndef G10_BDUS_PROXY_H_
#define G10_BDUS_PROXY_H_

#include <map>
#include <giomm.h>

#include "ProxyConnection.h"
#include "saftbus.h"

namespace saftbus
{

	enum ProxyFlags
	{
		PROXY_FLAGS_NONE,
	};


	class Proxy : public Glib::Object//Base
	{
	public:
		Proxy(saftbus::BusType  	bus_type,
			const Glib::ustring&  	name,
			const Glib::ustring&  	object_path,
			const Glib::ustring&  	interface_name,
			const Glib::RefPtr< InterfaceInfo >&  	info = Glib::RefPtr< InterfaceInfo >(),
			ProxyFlags  	flags = PROXY_FLAGS_NONE 
		);

		using MapChangedProperties = std::map<Glib::ustring, Glib::VariantBase>;

		void get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const ;

		virtual void 	on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< Glib::ustring >& invalidated_properties);
		virtual void 	on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters);
		Glib::RefPtr<saftbus::ProxyConnection> get_connection() const;

		Glib::ustring get_object_path() const;
		Glib::ustring get_name() const;

		const Glib::VariantContainerBase& call_sync(std::string function_name, Glib::VariantContainerBase query);

	private:
		static Glib::RefPtr<saftbus::ProxyConnection> _connection;
		static bool _connection_created;
	};

}


#endif
