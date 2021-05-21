/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include "core.h"
#include <stdexcept>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


#include <sys/file.h> // for flock()

namespace saftbus
{

	int _debug_level = 0;

	void block_signal(int signal_to_block /* i.e. SIGPIPE */ )
	{
			sigset_t set;
			sigset_t old_state;

			// get the current state
			sigprocmask(SIG_BLOCK, NULL, &old_state);

			// add signal_to_block to that existing state
			set = old_state;
			sigaddset(&set, signal_to_block);

			// block that signal also
			sigprocmask(SIG_BLOCK, &set, NULL);
	}
	void init()
	{
	}

	int write_all(int fd, const void *buffer, int size)
	{
		const char *ptr = static_cast<const char*>(buffer);
		int n_written;
		do {
			// calls to write are limited to 100 kB 
			// to avoid "message too long error" 
			// larger buffers are split into multiple calls to ::write
			int size_chunk = std::min(size,100000); 
			size -= size_chunk;
			n_written = 0;
			do {
				int result = ::write(fd, ptr, size_chunk-n_written);
				if (result > 0)
				{
					ptr       += result;
					n_written += result;
				}
				else
				{
					if (result == 0) { 
						throw std::runtime_error("saftbus write_all: could not write to pipe");
					} else {
						throw std::runtime_error((std::string("saftbus write_all: ") + std::string(strerror(errno))).c_str());
					} 
				}
				// if write was interrupted, not all of size was sent... 
				// ... just continue calling ::write() with updated buffer pointer and size
			} while (n_written < size_chunk);
		} while(size > 0);
		return n_written;
	}

	int read_all(int fd, void *buffer, int size)
	{
		//std::cerr << "read_all(buffer, " << size << ") called " << std::endl;
		char* ptr = static_cast<char*>(buffer);
		int n_read = 0;
		do { 
			int result = ::read(fd, ptr, size-n_read);//, MSG_DONTWAIT); 
			//std::cerr << "read result = " << result << " " << std::endl;
			if (result > 0)
			{
				ptr    += result;
				n_read += result;
			} else {
				if (result == 0) {
					if (n_read) {// if we already got some of the data, we try again
						std::cerr << "n_read = " << n_read << " ... that is why we continue ( size = " << size << " ) " << std::endl;
						continue;;
					} else {
						return -1; // -1 means end of file
					}
				}
				else             throw std::runtime_error((std::string(" read_all: ") + std::string(strerror(errno))).c_str());
			}
		} while (n_read < size);
		return n_read;
	}


	template<>
	int write<std::string>(int fd, const std::string & std_vector) {
		//if (_debug_level > 5) std::cerr << "std::string write " << std_vector << std::endl;
		std::string strg(std_vector);
		int32_t size = strg.size();
		int result = write_all(fd, static_cast<const void*>(&size), sizeof(int32_t));
		if (result == -1) return result;
		if (size > 0) return write_all(fd, static_cast<const void*>(&strg[0]), size*sizeof(decltype(strg.back())));
		return 1;
	}
	template<>
	int read<std::string>(int fd, std::string & std_vector) {
		//if (_debug_level > 5) std::cerr << "std::string read" << std::endl;
		std::string strg(std_vector);
		int32_t size;
		int result = read_all(fd, static_cast<void*>(&size), sizeof(int32_t));
		if (result == -1) return result;
		strg.resize(size);
		if (size > 0) result = read_all(fd, static_cast<void*>(&strg[0]), size*sizeof(decltype(strg.back())));
		std_vector = strg;
		return result;
	}



//  Sends given file descriptior via given socket
//  
//   @param socket to be used for fd sending
//   @param fd to be sent
//   @return sendmsg result
//  
//   @note socket should be (PF_UNIX, SOCK_DGRAM)
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

//   Receives file descriptor using given socket
//  
//   @param socket to be used for fd recepion
//   @return received file descriptor; -1 if failed
//  
//   @note socket should be (PF_UNIX, SOCK_DGRAM)
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
		std::cerr << "recvmsg failed with " << strerror(errno) << std::endl;
		return -1;
	}

	if (len == 0) {
		std::cerr << "recvmsg failed no data" << std::endl;
		return -1;
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	memmove(&fd, CMSG_DATA(cmsg), sizeof(int));
	return fd;
}




}
