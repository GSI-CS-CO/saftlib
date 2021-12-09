#include <saftbus.hpp>

#include <iostream>
#include <sstream>

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
#include <error.h>

namespace mini_saftlib {

	//  Sends given file descriptior via given socket
	//   @param socket to be used for fd sending
	//   @param fd to be sent
	//   @return sendmsg result
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