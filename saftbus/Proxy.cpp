#include "Proxy.h"

#include <iostream>

#include "saftbus.h"
#include "core.h"

namespace saftbus
{

Glib::RefPtr<saftbus::ProxyConnection> Proxy::_connection;
bool Proxy::_connection_created = false;

int Proxy::_global_id_counter = 0;
std::mutex Proxy::_id_counter_mutex;

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


	if (!static_cast<bool>(_connection)) {
		if (_debug_level > 5) std::cerr << "   this process has no ProxyConnection yet. Creating one now" << std::endl;
		_connection = Glib::RefPtr<saftbus::ProxyConnection>(new ProxyConnection);
		if (_debug_level > 5) std::cerr << "   ProxyConnection created" << std::endl;
	}

	// generate unique proxy id (unique for all running saftlib programs)
	{
		std::unique_lock<std::mutex> lock(_id_counter_mutex);
		++_global_id_counter;
		_global_id = 100*_global_id_counter + _connection->get_connection_id();
	}

	if (_debug_level > 5) std::cerr << "Proxy::Proxy(" << _global_id << " " << name << "," << object_path << "," << interface_name << ") called   _connection_created = " << static_cast<bool>(_connection) << std::endl;
	//std::cerr << "Proxy::Proxy() " << _global_id << "called " << std::endl;


//	_connection->register_proxy(interface_name, object_path, this);

	if (pipe(_pipe_fd) != 0) {
		std::cerr << "couldnt create pipe" << std::endl;
	}
	else {
		//std::cerr << "pipe is open _pipe_fd[0] = " << _pipe_fd[0] << "   _pipe_fd[1] = " << _pipe_fd[1] << std::endl;
		write(_connection->get_fd(), saftbus::SIGNAL_FD);
		sendfd(_connection->get_fd(), _pipe_fd[1]);	// send the writing endo of pipe
		write(_connection->get_fd(), _object_path);
		write(_connection->get_fd(), _interface_name);
		write(_connection->get_fd(), _global_id);
	}
	int message;
	read(_pipe_fd[0], message);
	//std::cerr << "got message through pipe" << message << std::endl;

    Glib::signal_io().connect(sigc::mem_fun(*this, &Proxy::dispatch), _pipe_fd[0], Glib::IO_IN | Glib::IO_HUP, Glib::PRIORITY_HIGH);
}

Proxy::~Proxy() 
{
	close(_pipe_fd[0]);
	close(_pipe_fd[1]);
	//std::cerr << "Proxy::~Proxy() " << _global_id << " called " << _global_id << std::endl;
	write(_connection->get_fd(), saftbus::SIGNAL_REMOVE_FD);
	write(_connection->get_fd(), _object_path);
	write(_connection->get_fd(), _interface_name);
	write(_connection->get_fd(), _global_id);
}


bool Proxy::dispatch(Glib::IOCondition condition)
{
	saftbus::MessageTypeS2C type;
	read(_pipe_fd[0], type);
	//std::cerr << "got signal through pipe " << std::endl;


	guint32 size;
	saftbus::read(_pipe_fd[0], size);
	if (_debug_level > 5) std::cerr << "      expecting message with size " << size << std::endl;
	std::vector<char> buffer(size);
	saftbus::read_all(_pipe_fd[0], &buffer[0], size);
	Glib::Variant<std::vector<Glib::VariantBase> > payload;
	deserialize(payload, &buffer[0], buffer.size());
	if (_debug_level > 5) std::cerr << "      got signal content " << payload.get_type_string() << " " << payload.print() << std::endl;
	Glib::Variant<Glib::ustring> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(0));
	Glib::Variant<Glib::ustring> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(1));
	Glib::Variant<Glib::ustring> signal_name    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(2));
	Glib::Variant<gint64> sec    = Glib::VariantBase::cast_dynamic<Glib::Variant<gint64> >(payload.get_child(3));
	Glib::Variant<gint64> nsec    = Glib::VariantBase::cast_dynamic<Glib::Variant<gint64> >(payload.get_child(4));
	Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>   (payload.get_child(5));



	// special treatment for property changes
	if (interface_name.get() == "org.freedesktop.DBus.Properties" && signal_name.get() == "PropertiesChanged")
	{
		// std::map<Glib::ustring, Glib::VariantBase> property_map;
		// parametrs.get(property_map 0)
		Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(0));
		//std::cerr << "derived_interface_name = " << derived_interface_name.print() << std::endl;
		if (_interface_name != derived_interface_name.get()) {
			std::cerr << "signal called with wrong interface name. expecting " << _interface_name << ",  got " << interface_name.get() << std::endl;
		}
		if (_object_path != object_path.get()) {
			std::cerr << "signal called with wrong object_path. expecting " << _object_path << ",  got " << object_path.get() << std::endl;
		}

		Glib::Variant<std::map<Glib::ustring, Glib::VariantBase> > property_map = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<Glib::ustring, Glib::VariantBase> > >(parameters.get_child(1));
		Glib::Variant<std::vector< Glib::ustring > > invalidated_properies = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector< Glib::ustring > > >(parameters.get_child(2));
				// std::cerr << "property_map = " << property_map.print() << std::endl;
				// std::cerr << "invalidated_properies = " << invalidated_properies.print() << std::endl;
	    struct timespec stop;
	    clock_gettime( CLOCK_REALTIME, &stop);
	    double dt = (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
	                - (1.0e6*sec.get() + 1.0e-3*nsec.get());
	    // deliver the signal
		on_properties_changed(property_map.get(), invalidated_properies.get());
	    //std::cerr << "signal flight time = " << dt << " us" << std::endl;
	}
	else // all other signals)
	{
		if (_interface_name != interface_name.get()) {
			std::cerr << "signal called with wrong interface name. expecting " << _interface_name << ",  got " << interface_name.get() << std::endl;
		}
		if (_object_path != object_path.get()) {
			std::cerr << "signal called with wrong object_path. expecting " << _object_path << ",  got " << object_path.get() << std::endl;
		}
	    struct timespec stop;
	    clock_gettime( CLOCK_REALTIME, &stop);
	    double dt = (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
	                - (1.0e6*sec.get() + 1.0e-3*nsec.get());
		on_signal("de.gsi.saftlib", signal_name.get(), parameters);
	    //std::cerr << "signal flight time = " << dt << " us" << std::endl;
	}

	return true;
}




void Proxy::get_cached_property (Glib::VariantBase& property, const Glib::ustring& property_name) const 
{
	if (_debug_level > 5) std::cerr << "Proxy::get_cached_property(" << property_name << ") called" << std::endl;

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
	if (_debug_level > 5) std::cerr << "Proxy::on_properties_changed() called" << std::endl;
}
void Proxy::on_signal (const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{
	if (_debug_level > 5) std::cerr << "Proxy::on_signal() called" << std::endl;
}
Glib::RefPtr<saftbus::ProxyConnection> Proxy::get_connection() const
{
	if (_debug_level > 5) std::cerr << "Proxy::get_connection() called " << std::endl;
	return _connection;
}

Glib::ustring Proxy::get_object_path() const
{
	if (_debug_level > 5) std::cerr << "Proxy::get_object_path() called" << std::endl;
	return _object_path;
}
Glib::ustring Proxy::get_name() const
{
	if (_debug_level > 5) std::cerr << "Proxy::get_name() called" << std::endl;
	return _name;
}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, const Glib::VariantContainerBase &query)
{
	//std::cerr << "Proxy::call_sync(" << function_name << ") called" << std::endl;
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
