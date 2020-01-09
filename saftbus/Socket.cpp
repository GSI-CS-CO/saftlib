#include "Socket.h"
#include "Connection.h"

#include <iostream>
#include <utility>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "core.h"

namespace saftbus
{

void Socket::close_connection()
{
	close(_new_socket);
}

bool Socket::get_active()
{
	return _active;
}

Socket::Socket(Connection *server_connection, int fd)
	: _server_connection(server_connection)
	, _new_socket(fd)
	, _active(false)
{
	Slib::signal_io().connect(sigc::bind(sigc::mem_fun(_server_connection, &Connection::dispatch), this), _new_socket, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_HIGH);
	_active = true;
}

int Socket::get_fd()
{
	return _new_socket;
}
std::string& Socket::saftbus_id()
{
	return _saftbus_id;
}


} // namespace saftbus