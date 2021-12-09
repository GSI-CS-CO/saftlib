#ifndef MINI_SAFTLIB_SAFTBUS_
#define MINI_SAFTLIB_SAFTBUS_

namespace mini_saftlib {

	int sendfd(int socket, int fd);
	int recvfd(int socket);

}

#endif
