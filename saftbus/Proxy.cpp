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


	_connection->register_proxy(interface_name, object_path, this);

	if (pipe(_pipe_fd) != 0) {
		std::cerr << "couldnt create pipe" << std::endl;
	}
	else {
		std::cerr << "pipe is open _pipe_fd[0] = " << _pipe_fd[0] << "   _pipe_fd[1] = " << _pipe_fd[1] << std::endl;
		write(_connection->get_fd(), saftbus::SIGNAL_FD);
		sendfd(_connection->get_fd(), _pipe_fd[1]);	// send the writing endo of pipe
		write(_connection->get_fd(), _object_path);
		write(_connection->get_fd(), _interface_name);
		write(_connection->get_fd(), _global_id);
	}
	int message;
	read(_pipe_fd[0], message);
	std::cerr << "got message through pipe" << message << std::endl;

    //Glib::signal_io().connect(sigc::mem_fun(*this, &Proxy::dispatch), _pipe_fd[1], Glib::IO_IN | Glib::IO_HUP, Glib::PRIORITY_HIGH);
}

Proxy::~Proxy() 
{
	close(_pipe_fd[0]);
	close(_pipe_fd[1]);
	std::cerr << "Proxy::~Proxy() called " << _global_id << std::endl;
	write(_connection->get_fd(), saftbus::SIGNAL_REMOVE_FD);
	write(_connection->get_fd(), _object_path);
	write(_connection->get_fd(), _interface_name);
	write(_connection->get_fd(), _global_id);
}


bool Proxy::dispatch(Glib::IOCondition condition)
{
	saftbus::MessageTypeS2C type;
	read(_pipe_fd[1], type);
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
