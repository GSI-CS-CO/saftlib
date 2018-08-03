#ifndef SERVER_SOCKET_H_
#define SERVER_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "giomm.h"

namespace saftbus
{

class Connection;

class Socket
{
	public:
		Socket(const std::string & name, Connection *server_connection); // if server_connection is 0 then we are a client
		~Socket();

		bool accept_connection(Glib::IOCondition condition);

		void wait_for_client(); 
		void close_connection();

		int get_fd();

		bool get_active();

		std::string get_filename();
		Glib::ustring& saftbus_id();

	private:
		//int _debug_level = 0;

		std::string _filename;
		int _create_socket, _new_socket; 
		struct sockaddr_un _address;
		socklen_t _addrlen;

		Connection *_server_connection;
		Glib::ustring _saftbus_id;


		bool _active;
};


} // namespace saftbus

#endif
