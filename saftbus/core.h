#ifndef SAFTBUS_CORE_H_
#define SAFTBUS_CORE_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

namespace saftbus
{

class UnSocket
{
public:
	UnSocket(const std::string &name, bool server);
	~UnSocket();

//	void accept();

private:

	int                _lock;
	int                _socket;
	int                _accepted;
	struct sockaddr_un _address;
};

}

#endif
