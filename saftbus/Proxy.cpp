#include "Proxy.h"

#include <iostream>

#include <unistd.h>

#include "saftbus.h"
#include "core.h"


namespace saftbus
{

std::shared_ptr<saftbus::ProxyConnection> Proxy::_connection;

int Proxy::_global_id_counter = 0;
std::mutex Proxy::_id_counter_mutex;

Proxy::Proxy(saftbus::BusType  	   bus_type,
             const std::string&  name,
             const std::string&  object_path,
             const std::string&  interface_name,
             const std::shared_ptr< InterfaceInfo >& info,
             //ProxyFlags            flags,
             saftlib::SignalGroup &signalGroup)
	: _name(name)
	, _object_path(object_path)
	, _interface_name(interface_name)
{
	//std::cerr << "saftbus::Proxy(" << object_path << ")" << std::endl;
	// if there is no ProxyConnection for this process yet we need to create one
	if (!static_cast<bool>(_connection)) {
		_connection = std::shared_ptr<saftbus::ProxyConnection>(new ProxyConnection);
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
	if (&signalGroup != &saftlib::noSignals) {
		try {
			if (pipe(_pipe_fd) != 0) {
				throw std::runtime_error("Proxy constructor: could not create pipe for signal transmission");
			}

			// send the writing end of a pipe to saftd 
			_connection->send_proxy_signal_fd(_pipe_fd[1], _object_path, _interface_name, _global_id);
			char ping;
			saftbus::read(_pipe_fd[0], ping);
			//std::cerr << "got ping after sending pipe: " << ping << std::endl;
		} catch(...) {
			std::cerr << "Proxy::~Proxy() exception" << std::endl;
		}

		// this Proxy will be part of the given SignalGroup.
		// saftlib::globalSignalGroup is the default
		signalGroup.add(this);
	}
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
		std::cerr << "Proxy::~Proxy() exception: " << e.what() << std::endl;
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
	    struct timespec start_read_time;
	    clock_gettime( CLOCK_REALTIME, &start_read_time);

		//std::cerr << "Proxy::dispatch() called" << std::endl;
		// read type and size of signal
		saftbus::MessageTypeS2C type;
		uint32_t                 size;
		saftbus::read(_pipe_fd[0], type);
		saftbus::read(_pipe_fd[0], size);

		// prepare buffer of the right size for the incoming data
		//std::vector<char> buffer(size);
		//saftbus::read_all(_pipe_fd[0], &buffer[0], size);

		// de-serialize using saftbus::Serial
		//std::cerr << "Proxy::dispatch() read payload" << std::endl;
		Serial payload;
		payload.data().resize(size);
		saftbus::read_all(_pipe_fd[0], &payload.data()[0], size);
		//std::cerr << "Proxy::dispatch() payload size = " << payload.get_size() << std::endl;
		std::string object_path;
		std::string interface_name;
		std::string signal_name;
	    struct timespec start_time;
		bool create_statistics;
		Serial parameters;
		payload.get_init();
		payload.get(object_path);
		payload.get(interface_name);
		payload.get(signal_name);
		payload.get(start_time.tv_sec);
		payload.get(start_time.tv_nsec);
		payload.get(create_statistics);
		payload.get(parameters);


		// if we don't get the expected _object path, saftd probably messed up the pipe lookup
		if (_object_path != object_path) {
			std::ostringstream msg;
			msg << "Proxy::dispatch() : signal with wrong object_path: expecting " 
			    << _object_path 
			    << ",  got " 
			    << object_path;
			throw std::runtime_error(msg.str());
		}

		double signal_flight_time;
		double signal_read_time;

		// special treatment for property changes
		//if (interface_name.get() == "org.freedesktop.DBus.Properties" && signal_name.get() == "PropertiesChanged")
		if (interface_name == "org.freedesktop.DBus.Properties" && signal_name == "PropertiesChanged")
		{	
			// nothing ...
		}
		else // all other signals
		{
			//std::cerr << "Proxy::dispatch() a normal signal" << std::endl;
			// if we don't get the expected _interface_name, saftd probably messed up the pipe lookup		
			//if (_interface_name != interface_name.get()) {
			if (_interface_name != interface_name) {
				std::ostringstream msg;
				msg << "Proxy::dispatch() : signal with wrong interface name: expected " 
				    << _interface_name 
				    << ",  got " 
				    //<< interface_name.get();
				    << interface_name;
				throw std::runtime_error(msg.str());
			}
			// get the signal flight stop time right before we call the signal handler from the Proxy object
		    struct timespec stop;
		    clock_gettime( CLOCK_REALTIME, &stop);
		    signal_flight_time = (1.0e6*stop.tv_sec       + 1.0e-3*stop.tv_nsec) 
		                       - (1.0e6*start_time.tv_sec + 1.0e-3*start_time.tv_nsec);

		    signal_read_time = (1.0e6*stop.tv_sec            + 1.0e-3*stop.tv_nsec) 
		                     - (1.0e6*start_read_time.tv_sec + 1.0e-3*start_read_time.tv_nsec);

		    if (create_statistics && signal_read_time > 200) { // if reading takes more than 200 us => Report!
		    	std::cerr << "Proxy::dispatch() " << _name << " " << _interface_name << " " << _object_path << "  unusual long reading time for signal of " << signal_read_time << " us" << std::endl;
		    }

			// report the measured signal flight time to saftd
		    try {
		    	if (create_statistics) { // do this only if switched on
			    	_connection->send_signal_flight_time(signal_flight_time);
			    }
			    // deliver the signal: call the signal handler of the derived class 
			    //std::cerr << "Proxy::dispatch() call on_signal" << std::endl;
				on_signal("de.gsi.saftlib", signal_name, parameters);
			} catch (std::runtime_error &e) {
				throw e;
			} catch(...) {
				std::cerr << "Proxy::dispatch() : on_signal threw " << std::endl;
			}
		}
	} catch(std::runtime_error &e) {
		throw e;		
	} catch (std::exception &e) {
		std::cerr << "Proxy::dispatch() : exception : " << e.what() << std::endl;
	}


	return true;
}

void Proxy::get_cached_property (Serial& property, const std::string& property_name) const 
{
	// this is not implemented yet and it is questionable if this is beneficial in case of saftlib
	return; // empty response
}

void Proxy::on_properties_changed (const MapChangedProperties& changed_properties, const std::vector< std::string >& invalidated_properties)
{
	// this will be overloaded by the derived Proxy class
}
void Proxy::on_signal (const std::string& sender_name, const std::string& signal_name, const saftbus::Serial& parameters)
{
	// this will be overloaded by the derived Proxy class
}
std::shared_ptr<saftbus::ProxyConnection> Proxy::get_connection() const
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

const Serial& Proxy::call_sync(std::string function_name, const Serial &query)
{
	//// please keep this code as a monument of insanity
	//// call the Connection::call_sync in a special way that cast the result in a special way. 
	////   Without this cast the generated Proxy code cannot handle the resulting variant type.
	// _result = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
	// 		  	Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<Glib::VariantBase> > >(
	// 	               _connection->call_sync(_object_path, 
	// 	                                      _interface_name,
	// 	                                      function_name,
	// 	                                      query)).get_child(0));
	// return _result;

	const Serial &result = _connection->call_sync(_object_path, 
		                          _interface_name,
		                          function_name,
		                          query);

	// std::cerr << "Proxy::call_sync(" << function_name << ") received Serial" << std::endl;
	// result.print();
	// std::cerr << "Proxy::call_sync(" << function_name << ") done " << std::endl;
	return result;
}



}
