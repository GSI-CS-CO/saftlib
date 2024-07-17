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

#include <saftbus.hpp>

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <error.h>

namespace saftbus {

	int write_all(int fd, const char *buffer, int size)
	{
		// Here is some storage that is used in case "write_all" is not possible
		// if for example the client is not reading on the other end of the socket.
		// Remaining part of the buffer that was wasn't sent is stored and sent whenever
		// write_all is called on the same file descriptor.
		static std::map<int, std::vector<char> > rest_of_data;

		// First check if there is remaining data for that file descriptor an send this 
		// first.
		if (rest_of_data[fd].size() > 0) {
			const char *ptr = &rest_of_data[fd][0];
			int rest_size = rest_of_data[fd].size();
			int written_total = 0;
			do {
				int size_chunk = std::min(rest_size,100000);
				int written_chunk = 0;
				do {
					int result = ::write(fd, ptr, size_chunk-written_chunk);
					if (result > 0)	{
						ptr           += result;
						written_chunk += result;
					}
					else {
						return result;
					}
				} while (written_chunk < size_chunk);
				rest_size     -= size_chunk;
				written_total += written_chunk;
			} while(rest_size > 0);
			rest_of_data[fd].clear();
		}

		// Only then send the data in "buffer"
		const char *ptr = buffer;
		int written_total = 0;
		do {
			// calls to write are limited to 100 kB 
			// to avoid "message too long error" 
			// larger buffers are split into multiple calls to ::write
			int size_chunk = std::min(size,100000); 
			int written_chunk = 0;
			do {
				struct pollfd pfd;
				pfd.fd = fd;
				pfd.events = POLLOUT;
				int pollresult;
				if ((pollresult=poll(&pfd, 1, 1)) <= 0) {
					rest_of_data[fd].insert(rest_of_data[fd].end(),ptr,ptr+size);
					return -1;
				}

				int result = ::write(fd, ptr, size_chunk-written_chunk);
				if (result > 0)	{
					ptr           += result;
					written_chunk += result;
				}
				else {
					return result;
				}
			} while (written_chunk < size_chunk);
			size          -= size_chunk;
			written_total += written_chunk;
		} while(size > 0);
		return written_total;
	}

	int read_all(int fd, char *buffer, int size)
	{
		char* ptr = buffer;
		int read_total = 0;
		do { 
			int result = ::read(fd, ptr, size-read_total); 
			if (result > 0)
			{
				ptr        += result;
				read_total += result;
			} else {
				return result;
			}
		} while (read_total < size);
		return read_total;
	}

	bool Serializer::write_to(int fd) {
		bool result = write_to_no_init(fd);
		put_init();
		return result;
	}
	bool Serializer::write_to_no_init(int fd) {
		int size = _data.size();
		int result;
		result = write_all(fd, (char*)&size, sizeof(size));
		if (result < (int)sizeof(size)) {
			//std::cerr << "write_all returned " << result << ". Expected result " << sizeof(size) << ". errno: " << strerror(errno) << std::endl;
			return false;
		}
		// std::cerr << "write_to " << fd << " so many bytes " << size << std::endl;
		result = write_all(fd, (char*)&_data[0], size);
		if (result < size) {
			//std::cerr << "write_all returned " << result << ". Expected result " << size << ". errno: " << strerror(errno) << std::endl;
			return false;
		}
		// std::cerr << "wrote " << size << " bytes to fd " << fd << std::endl;
		// put_init();
		return true;
	}
	bool Serializer::empty()
	{
		return _data.empty();
	}
	void Serializer::put_init()
	{
		_data.clear();
	}

	bool Deserializer::read_from(int fd) {
		int size;
		int result;
		result = read_all(fd, (char*)&size, sizeof(size));
		// std::cerr << "read_from " << fd << " so many bytes: " << size << std::endl;
		if (result < (int)sizeof(size)) {
			//std::cerr << "read_all returned " << result << ". Expected result " << sizeof(size) << ". errno: " << strerror(errno) << std::endl;
			return false;
		}
		_data.resize(size);
		result = read_all(fd, (char*)&_data[0], size);
		if (result < size) {
			//std::cerr << "read_all returned " << result << ". Expected result " << size << ". errno: " << strerror(errno) << std::endl;
			return false;
		}
		get_init();
		// std::cerr << "read " << size << " bytes from fd " << fd << std::endl;
		return true;
	}
	void Deserializer::save() const
	{
		_saved_iter = _iter;
	}
	void Deserializer::restore() const
	{
		_iter = _saved_iter;
	}
	// has to be called before any call to get()
	void Deserializer::get_init() const
	{
		_iter = _data.begin();
	}

	int sendfd(int socket, int fd) {
		char dummy = '$';
		struct msghdr msg;
		struct iovec iov;

		char cmsgbuf[CMSG_SPACE(sizeof(int))];

		iov.iov_base = &dummy;
		iov.iov_len = sizeof(dummy);

		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_flags = 0;
		msg.msg_control = cmsgbuf;
		msg.msg_controllen = CMSG_LEN(sizeof(int));

		struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));

		*(int*) CMSG_DATA(cmsg) = fd;

		int ret = sendmsg(socket, &msg, 0);

		if (ret == -1) {
			std::cerr << "sendmsg failed with " << strerror(errno) << std::endl;
		}

		return ret;
	}


	int recvfd(int socket) {
		int len;
		int fd;
		char buf[1];
		struct iovec iov;
		struct msghdr msg;
		struct cmsghdr *cmsg;
		char cms[CMSG_SPACE(sizeof(int))];

		iov.iov_base = buf;
		iov.iov_len = sizeof(buf);

		msg.msg_name = 0;
		msg.msg_namelen = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_flags = 0;
		msg.msg_control = (caddr_t) cms;
		msg.msg_controllen = sizeof cms;

		len = recvmsg(socket, &msg, 0);

		if (len < 0) {
			return -1;
		}

		if (len == 0) {
			return -1;
		}

		cmsg = CMSG_FIRSTHDR(&msg);
		memmove(&fd, CMSG_DATA(cmsg), sizeof(int));
		return fd;
	}


}