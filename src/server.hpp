/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#ifndef SAFTBUS_SERVER_CONNECTION_HPP_
#define SAFTBUS_SERVER_CONNECTION_HPP_

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <unistd.h>

namespace saftbus {

	class Container;

	/// provide one named UNIX domain socket in the file system and handle client request on that socket
	/// 
	/// In its constructor, the @ServerConnection creates a UNIX domain socket in the file system and 
	/// listens for incoming connections. If a client connects to that socket, it is expected to send 
	/// one file descriptor which must be one of the returned elements of the socketpair system call.
	/// The @ServerConnection maintains this this descriptor until it detects that the client hung up.
	/// Since file descriptors are unique integer numbers, they are used to uniquely identify a client.
	class ServerConnection {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		ServerConnection(const std::vector<std::pair<std::string, std::vector<std::string> > > &plugins_and_args = std::vector<std::pair<std::string, std::vector<std::string> > >(), 
			             const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ServerConnection();

		void register_signal_id_for_client(int client_id, int signal_id);
		void unregister_signal_id_for_client(int client_id, int signal_id);

		int get_calling_client_id();

		struct ClientInfo {
			pid_t process_id;
			int client_fd;
			std::map<int,int> signal_fds;
		};
		std::vector<ClientInfo> get_client_info();
	};

}


#endif
