#include "eb-source.hpp"

#include <chrono>
#include <algorithm>
#include <array>
#include <thread>
#include <iostream>
#include <cstring>
#include <cassert>

namespace eb_plugin {


	EB_Source::EB_Source(etherbone::Socket socket_)
	 : socket(socket_)
	{
	}

	EB_Source::~EB_Source()
	{
	  // This is not needed (and emits warnings) because all polls already removed by glib
	  // for (fd_map::iterator i = fds.begin(); i != fds.end(); ++i)
	  //   remove_poll(i->second);
	}

	int EB_Source::add_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
	  std::cerr << "EB_Source::add_fd(" << fd << ")" << std::endl;
	  EB_Source* self = (EB_Source*)data;
	  
	  struct pollfd pfd;
	  pfd.fd = fd;
	  pfd.events = POLLERR;

	  std::pair<fd_map::iterator, bool> res = 
	    self->fds.insert(fd_map::value_type(fd, pfd));
	  
	  if (res.second) // new element; add to poll
	    self->add_poll(res.first->second);
	  
	  int flags = POLLERR | POLLHUP;
	  if ((mode & EB_DESCRIPTOR_IN)  != 0) flags |= POLLIN;
	  if ((mode & EB_DESCRIPTOR_OUT) != 0) flags |= POLLOUT;
	  
	  res.first->second.events |= flags;
	  return 0;
	}

	int EB_Source::get_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
	  std::cerr << "EB_Source::get_fd" << std::endl;
	  return 0;
	  EB_Source* self = (EB_Source*)data;
	  
	  fd_map::iterator i = self->fds.find(fd);
	  if (i == self->fds.end()) return 0;
	  
	  int flags = i->second.revents;
	  
	  return 
	    ((mode & EB_DESCRIPTOR_IN)  != 0 && (flags & (POLLIN  | POLLERR | POLLHUP)) != 0) ||
	    ((mode & EB_DESCRIPTOR_OUT) != 0 && (flags & (POLLOUT | POLLERR | POLLHUP)) != 0);
	}

	static int no_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
	  return 0;
	}

	bool EB_Source::prepare(std::chrono::milliseconds &timeout_ms)
	{
	  std::cerr << "EB_Source::prepare" << std::endl;
	  // Retrieve cached current time
	  auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
	  auto epoch  = now.time_since_epoch();
	  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
	  
	  // Work-around for no TX flow control: flush data now
	  socket.check(now_ms, 0, &no_fd);
	  
	  // Clear the old requested FD status
	  for (fd_map::iterator i = fds.begin(); i != fds.end(); ++i)
	    i->second.events = POLLERR;
	  
	  // Find descriptors we need to watch
	  socket.descriptors(this, &EB_Source::add_fd);
	  
	  // Eliminate any descriptors no one cares about any more
	  fd_map::iterator i = fds.begin();
	  while (i != fds.end()) {
	    fd_map::iterator j = i;
	    ++j;
	    if ((i->second.events & POLLHUP) == 0) {
	      remove_poll(i->second);
	      fds.erase(i);
	    }
	    i = j;
	  }
	  
	  // Determine timeout
	  uint32_t timeout = socket.timeout();
	  if (timeout) {
	  	uint64_t ms = static_cast<int64_t>(timeout)*1000 - now_ms;
	    if (ms < 0) {
	    	timeout_ms = std::chrono::milliseconds(0);
	    } else {
	    	timeout_ms = std::chrono::milliseconds(ms);
	    }
	  } else {
	    timeout_ms = std::chrono::milliseconds(-1);
	  }
	  
	  return (timeout_ms.count() == 0); // true means immediately ready
	}

	bool EB_Source::check()
	{
	  std::cerr << "EB_Source::check" << std::endl;
	  bool ready = false;
	  
	  // Descriptors ready?
	  for (fd_map::const_iterator i = fds.begin(); i != fds.end(); ++i)
	    ready |= (i->second.revents & i->second.events) != 0;
	  
	  // Timeout ready?
	  auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
	  auto epoch  = now.time_since_epoch();
	  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
	  ready |= socket.timeout() >= now_ms/1000;
	  
	  return ready;
	}

	bool EB_Source::dispatch()
	{
	  std::cerr << "EB_Source::dispatch" << std::endl;

	  auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
	  auto epoch  = now.time_since_epoch();
	  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
	  // Process any pending packets
	  socket.check(now_ms/1000, this, &EB_Source::get_fd);
	  
	  return true;
	}


}