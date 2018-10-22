#ifndef G10_BDUS_PROXY_H_
#define G10_BDUS_PROXY_H_

#include <map>
#include <giomm.h>
#include <thread>

#include "saftbus.h"

namespace saftbus
{

	enum ProxyFlags
	{
		PROXY_FLAGS_NONE = 0,
		PROXY_FLAGS_ACTIVE_WAIT_FOR_SIGNAL = 1, // if this is set, the signals are not 
		                                        // delivered by a Glib::MainLoop, but 
		                                        // by a call to 
	};

	class ProxyConnection;


	// This class mimics the Gio::DBus::Proxy class interface. It is different from the Gio::DBus::Proxy 
	// in that it depends on a dedicated saftbus::ProxyConnection class and not the saftbus::Connection class.
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
		~Proxy();

		using MapChangedProperties = std::map<Glib::ustring, Glib::VariantBase>;

		void get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const ;

		virtual void on_properties_changed(const MapChangedProperties& changed_properties, const std::vector< Glib::ustring >& invalidated_properties);
		virtual void on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters);
		Glib::RefPtr<saftbus::ProxyConnection> get_connection() const;

		Glib::ustring get_object_path() const;
		Glib::ustring get_name() const;

		const Glib::VariantContainerBase& call_sync(std::string function_name, const Glib::VariantContainerBase &query);


		static void wait_for_signal(const std::vector<Glib::RefPtr<Proxy> > &proxy_band);

		int get_reading_end_of_signal_pipe();

	public:
		bool dispatch(Glib::IOCondition condition);

	private:

		static Glib::RefPtr<saftbus::ProxyConnection> _connection;

		static int _global_id_counter;
		static std::mutex _id_counter_mutex;
		int _global_id;

		Glib::ustring _name;
		Glib::ustring _object_path;
		Glib::ustring _interface_name;


		Glib::Variant<std::vector<Glib::VariantBase> > _call_sync_result;
		std::vector<char> _call_sync_result_buffer;

		Glib::VariantContainerBase _result;

		// A Unix pipe that is private between a Saftlib device and a Saftlib Proxy object.
		// It serves as independent fast channel for signals from the device to the Proxy.
		// The Proxy constructor creates it and sends the writing end of the pipe through the 
		// socket connection
		int _pipe_fd[2];

		sigc::connection _signal_connection_handle;
	};

}


#endif
