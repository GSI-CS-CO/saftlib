#include "Proxy.h"

#include <iostream>

#include "saftbus.h"
#include "core.h"

namespace saftbus
{

Glib::RefPtr<saftbus::ProxyConnection> Proxy::_connection;

int Proxy::_global_id_counter = 0;
std::mutex Proxy::_id_counter_mutex;

Proxy::Proxy(saftbus::BusType  	   bus_type,
             const std::string&  name,
             const std::string&  object_path,
             const std::string&  interface_name,
             const Glib::RefPtr< InterfaceInfo >& info,
             //ProxyFlags            flags,
             saftlib::SignalGroup &signalGroup)
	: _name(name)
	, _object_path(object_path)
	, _interface_name(interface_name)
{
	//std::cerr << "saftbus::Proxy(" << object_path << ")" << std::endl;
	// if there is no ProxyConnection for this process yet we need to create one
	if (!static_cast<bool>(_connection)) {
		_connection = Glib::RefPtr<saftbus::ProxyConnection>(new ProxyConnection);
	}

	// generate unique proxy id (unique for all running saftlib programs)
	{
		std::unique_lock<std::mutex> lock(_id_counter_mutex);
		++_global_id_counter;
		// thjs assumes there are no more than 100 saftbus sockets available ever
		// (connection_id is the socket number XX in the socket filename "/tmp/saftbus_XX")
		_global_id = 100*_global_id_counter + _connection->get_connection_id();
	}

	// create a pipe through which we will receive signals from the saftd
	try {
		if (pipe(_pipe_fd) != 0) {
			throw std::runtime_error("Proxy constructor: could not create pipe for signal transmission");
		}

		// send the writing end of a pipe to saftd 
		_connection->send_proxy_signal_fd(_pipe_fd[1], _object_path, _interface_name, _global_id);
	} catch(...) {
		std::cerr << "Proxy::~Proxy() threw" << std::endl;
	}

	// this Proxy will be part of the given SignalGroup.
	// saftlib::globalSignalGroup is the default
	signalGroup.add(this);
}

Proxy::~Proxy() 
{
	//std::cerr << "saftbus::Proxy::~Proxy(" << _object_path << ")" << std::endl;
	_signal_connection_handle.disconnect();

	// remove this Proxy from the globalSignalGroup. if the Proxy was not 
	// attached to the globalSignalGroup nothing happens
	saftlib::globalSignalGroup.remove(this);

	// free all resources ...
	try {
		_connection->remove_proxy_signal_fd(_object_path, _interface_name, _global_id);
		close(_pipe_fd[0]);
		close(_pipe_fd[1]);
	} catch(std::exception &e) {
		std::cerr << "Proxy::~Proxy() threw: " << e.what() << std::endl;
	}
}

int Proxy::get_reading_end_of_signal_pipe()
{
	return _pipe_fd[0];
}


bool Proxy::dispatch(Slib::IOCondition condition)
{
	// this method is called from the Glib::MainLoop whenever there is signal data in the pipe
	try {

		// read type and size of signal
		saftbus::MessageTypeS2C type;
		guint32                 size;
		saftbus::read(_pipe_fd[0], type);
		saftbus::read(_pipe_fd[0], size);

		// prepare buffer of the right size for the incoming data
		std::vector<char> buffer(size);
		saftbus::read_all(_pipe_fd[0], &buffer[0], size);

		// de-serialize the data using the Glib::Variant infrastructure
		Glib::Variant<std::vector<Glib::VariantBase> > payload;
		deserialize(payload, &buffer[0], buffer.size());
		// read content from the variant type (this works because we know what saftd will send us)
		Glib::Variant<std::string> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> > (payload.get_child(0));
		Glib::Variant<std::string> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> > (payload.get_child(1));
		Glib::Variant<std::string> signal_name    = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> > (payload.get_child(2));
		// the following two items are for signal flight time measurement (the time when the signal was sent)
		Glib::Variant<gint64> sec                   = Glib::VariantBase::cast_dynamic<Glib::Variant<gint64> >        (payload.get_child(3));
		Glib::Variant<gint64> nsec                  = Glib::VariantBase::cast_dynamic<Glib::Variant<gint64> >        (payload.get_child(4));
		Glib::Variant<gint32> create_statistics     = Glib::VariantBase::cast_dynamic<Glib::Variant<gint32> >        (payload.get_child(5));
		Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>    (payload.get_child(6));

		// if we don't get the expected _object path, saftd probably messed up the pipe lookup
		if (_object_path != object_path.get()) {
			std::ostringstream msg;
			msg << "Proxy::dispatch() : signal with wrong object_path: expecting " 
			    << _object_path 
			    << ",  got " 
			    << object_path.get();
			throw std::runtime_error(msg.str());
		}

		double signal_flight_time;

		// special treatment for property changes
		if (interface_name.get() == "org.freedesktop.DBus.Properties" && signal_name.get() == "PropertiesChanged")
		{	/*
			// in case of a property change, the interface name of the property 
			// that was changed (here we call it derived_interface_name) is embedded in the data
			Glib::Variant<std::string> derived_interface_name 
					= Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(parameters.get_child(0));
			// if we don't get the expected _interface_name, saftd probably messed up the pipe lookup		
			if (_interface_name != derived_interface_name.get()) {
				std::ostringstream msg;
				msg << "Proxy::dispatch() : signal with wrong interface name: expected " 
				    << _interface_name 
				    << ",  got " 
				    << derived_interface_name.get();
				throw std::runtime_error(msg.str());
			}

			// get the real data: which property has which value
			Glib::Variant<std::map<std::string, Glib::VariantBase> > property_map 
					= Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<std::string, Glib::VariantBase> > >
							(parameters.get_child(1));
			Glib::Variant<std::vector< std::string > > invalidated_properies 
					= Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector< std::string > > >
							(parameters.get_child(2));

			// get the signal flight stop time right before we call the signal handler from the Proxy object
		    struct timespec stop;
		    clock_gettime( CLOCK_REALTIME, &stop);
		    signal_flight_time = (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
		                       - (1.0e6*sec.get()     + 1.0e-3*nsec.get());
			// report the measured signal flight time to saftd
		    try {
		    	if (create_statistics.get()) { // do this only if switched on
		    		_connection->send_signal_flight_time(signal_flight_time);
		    	}
			    // deliver the signal: call the property changed handler of the derived class
				on_properties_changed(property_map.get(), invalidated_properies.get());
			} catch(...) {
				std::cerr << "Proxy::dispatch() : on_properties_changed threw " << std::endl;
			}
			*/
		}
		else // all other signals
		{
			// if we don't get the expected _interface_name, saftd probably messed up the pipe lookup		
			if (_interface_name != interface_name.get()) {
				std::ostringstream msg;
				msg << "Proxy::dispatch() : signal with wrong interface name: expected " 
				    << _interface_name 
				    << ",  got " 
				    << interface_name.get();
				throw std::runtime_error(msg.str());
			}
			// get the signal flight stop time right before we call the signal handler from the Proxy object
		    struct timespec stop;
		    clock_gettime( CLOCK_REALTIME, &stop);
		    signal_flight_time = (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
		                       - (1.0e6*sec.get()     + 1.0e-3*nsec.get());
			// report the measured signal flight time to saftd
		    try {
		    	if (create_statistics.get()) { // do this only if switched on
			    	_connection->send_signal_flight_time(signal_flight_time);
			    }
			    // deliver the signal: call the signal handler of the derived class 
				on_signal("de.gsi.saftlib", signal_name.get(), parameters);
			} catch(...) {
				std::cerr << "Proxy::dispatch() : on_signal threw " << std::endl;
			}
		}
	} catch (std::exception &e) {
		std::cerr << "Proxy::dispatch() : exception : " << e.what() << std::endl;
	}


	return true;
}

void Proxy::get_cached_property (Glib::VariantBase& property, const std::string& property_name) const 
{
	// this is not implemented yet and it is questionable if this is beneficial in case of saftlib
	return; // empty response
}

void Proxy::on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< std::string >& invalidated_properties)
{
	// this will be overloaded by the derived Proxy class
}
void Proxy::on_signal (const std::string& sender_name, const std::string& signal_name, const Glib::VariantContainerBase& parameters)
{
	// this will be overloaded by the derived Proxy class
}
Glib::RefPtr<saftbus::ProxyConnection> Proxy::get_connection() const
{
	return _connection;
}

std::string Proxy::get_object_path() const
{
	return _object_path;
}
std::string Proxy::get_name() const
{
	return _name;
}

const Glib::VariantContainerBase& Proxy::call_sync(std::string function_name, const Glib::VariantContainerBase &query)
{
	// call the Connection::call_sync in a special way that cast the result in a special way. 
	//   Without this cast the generated Proxy code cannot handle the resulting variant type.
	_result = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
			  	Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase> > >(
						_connection->call_sync(_object_path, 
		                			          _interface_name,
		                			          function_name,
		                			          query)).get_child(0));
	return _result;
}



}
