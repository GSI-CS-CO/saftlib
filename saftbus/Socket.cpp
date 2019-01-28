#include "Socket.h"
#include "Connection.h"

#include <iostream>
#include <utility>
#include <time.h>

#include "giomm.h"
#include "core.h"



namespace saftbus
{

void Socket::wait_for_client()
{
	_active = false;
	struct timespec start, stop;
	clock_gettime( CLOCK_REALTIME, &start);
	_create_socket = socket(AF_LOCAL, SOCK_SEQPACKET, 0);
	if (_create_socket > 0) {
		if (_debug_level > 5) std::cerr << "socket created" << std::endl;
	} // TODO: else { ... }
	unlink(_filename.c_str());
	_address.sun_family = AF_LOCAL;
	strcpy(_address.sun_path, _filename.c_str());
	if (_debug_level > 5) std::cerr << "bind to file " << _filename << std::endl;
	int bind_result = bind (_create_socket, (struct sockaddr *)&_address, sizeof(_address) );
	if (bind_result != 0) {
		if (_debug_level > 5) std::cerr << "port busy" << std::endl;
		throw std::runtime_error("port busy");
	}
	listen(_create_socket, 1);
	//_addrlen = sizeof(struct sockaddr_in);
	// the connection will wait for incoming calls
	try {
		Slib::signal_io().connect(sigc::mem_fun(*this, &Socket::accept_connection), _create_socket, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
	} catch(std::exception &e)
	{
		if (_debug_level > 5) std::cerr << "wait_for_client exception: " << e.what() << std::endl;
		throw;
	}
	clock_gettime( CLOCK_REALTIME, &stop);
	double dt = (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
						- (1.0e6*start.tv_sec + 1.0e-3*start.tv_nsec);

	if (_debug_level > 5) std::cerr << "wait_for_client dt = " << dt << " us" << std::endl;          

}

void Socket::close_connection()
{
	close(_new_socket);
}

bool Socket::get_active()
{
	return _active;
}

std::string Socket::get_filename()
{
	return _filename;
}

Socket::Socket(const std::string &name, Connection *server_connection)
	: _filename(name)
	, _server_connection(server_connection)
	, _active(false)

{
	// // the connection will wait for incoming calls
	// Slib::signal_io().connect(sigc::mem_fun(*this, &Socket::accept_connection), _create_socket, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
	wait_for_client();
}

bool Socket::accept_connection(Slib::IOCondition condition)
{
	std::cerr << "Socket::accept_connection() called" << std::endl;
	if (_active) {
		return false;
	}
	_addrlen = sizeof(struct sockaddr_in);
	_new_socket = accept(_create_socket, (struct sockaddr*) &_address, &_addrlen);
	//std::cerr << "listen = " << listen(_create_socket, 0) << std::endl;
	close(_create_socket);
	if (_new_socket > 0)
	{
		if (_debug_level > 5) std::cerr << "client connected " << std::endl;
	}
	std::cerr << "Socket::accept_connection() bind the dispatch function" << std::endl;
	Slib::signal_io().connect(sigc::bind(sigc::mem_fun(_server_connection, &Connection::dispatch), this), _new_socket, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_HIGH);
	_active = true;
	return false; // MainLoop should stop watching _create_socket file descriptor
}

int Socket::get_fd()
{
	return _new_socket;
}
std::string& Socket::saftbus_id()
{
	return _saftbus_id;
}


Socket::~Socket()
{
	close(_create_socket);
}

} // namespace saftbus