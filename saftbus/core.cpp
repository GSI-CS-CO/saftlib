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


#include <sys/file.h> // for flock()

namespace saftbus
{
/* uds_server.c 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd. h.>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024
#define UDS_FILE "/tmp/sock.uds"
int main (void) {
  int create_socket, new_socket;
  socklen_t addrlen;
  char *buffer = malloc (BUF);
  ssize_t size;
  struct sockaddr_un address;
  const int y = 1;
  printf ("\e[2J");
  if((create_socket=socket (AF_LOCAL, SOCK_STREAM, 0)) > 0)
    printf ("Socket wurde angelegt\n");
  unlink(UDS_FILE);
  address.sun_family = AF_LOCAL;
  strcpy(address.sun_path, UDS_FILE);
  if (bind ( create_socket,
             (struct sockaddr *) &address,
             sizeof (address)) != 0) {
    printf( "Der Port ist nicht frei – belegt!\n");
  }
  listen (create_socket, 5);
  addrlen = sizeof (struct sockaddr_in);
  while (1) {
     new_socket = accept ( create_socket,
                           (struct sockaddr *) &address,
                           &addrlen );
     if (new_socket > 0)
      printf ("Ein Client ist verbunden ...\n");
     do {
        printf ("Nachricht zum Versenden: ");
        fgets (buffer, BUF, stdin);
        send (new_socket, buffer, strlen (buffer), 0);
        size = recv (new_socket, buffer, BUF-1, 0);
        if( size > 0)
           buffer[size] = '\0';
        printf ("Nachricht empfangen: %s\n", buffer);
     } while (strcmp (buffer, "quit\n") != 0);
     close (new_socket);
  }
  close (create_socket);
  return EXIT_SUCCESS;
}*/


UnSocket::UnSocket(const std::string &name, bool server)
	: _lock(0), _socket(0), _accepted(0)
{
	if (name.size() == 0)
		return;

  if (!server) // check if file is locked by another client
  {
    std::cerr << "check if there is a lock on " << name << std::endl;
    _lock = open((name+".lock").c_str(), O_RDWR | O_CREAT | O_EXCL);
    if (_lock) { // success
      std::cerr << "lock created: " << _lock << std::endl;
      // if (flock(_lock, LOCK_EX | LOCK_NB) == 0) {
      //   // success ... nothing to be done        
      // }
      // else 
      // {
      //   std::string msg = "file could not be locked: ";
      //   msg.append(name);
      //   std::cerr << msg << std::endl;
      //   throw std::runtime_error(msg);
      // }
    }
    else 
    {
      std::string msg = "file could not be opened: ";
      msg.append(name);
      std::cerr << msg << std::endl;
      throw std::runtime_error(msg);
    }
  }


  if ((_socket = socket((server?AF_LOCAL:PF_LOCAL), SOCK_STREAM, 0)) > 0)
  {
    if (server) {
      unlink(name.c_str());
    }
    _address.sun_family = AF_LOCAL;
    strcpy(_address.sun_path, name.c_str());
    std::cerr << "socket created: " << name << " handle=" << _socket << std::endl;
    int result = 0;
    if (server) {
      result = bind(_socket, (struct sockaddr *) &_address, sizeof (_address));
    } else {
      result = connect(_socket, (struct sockaddr *) &_address, sizeof (_address));
    }
    if (result == 0)
    {
      std::cerr << (server?"bound":"connected") << " to socket: " << name << std::endl;
    }
    else
    {
      std::string msg = "could not ";
      msg.append(server?"bind":"connect");
      msg.append(" to socket: ");
      msg.append(name);
      std::cerr << msg << std::endl;
      throw std::runtime_error(msg);
    }
  }
  else
  {
    std::string msg = "socket could not be created: ";
    msg.append(name);
    std::cerr << msg << std::endl;
    throw std::runtime_error(msg);
  }

}

// int UnSocket::accept()
// {
//   return accept(_socket, (struct sockaddr*) &_address, sizeof(_address));
// }


UnSocket::~UnSocket()
{
	
}




}
