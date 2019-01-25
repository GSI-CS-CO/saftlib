/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <cstdint>
#include <memory>
#include <sigc++/sigc++.h>
#include <etherbone.h>
#include "eb-source.h"
#include "saftbus/Source.h"
#include "saftbus/PollFD.h"

class EB_Source : public Slib::Source
{
  public:
    static std::shared_ptr<EB_Source> create(etherbone::Socket socket);
    sigc::connection connect(const sigc::slot<bool>& slot);
    
    virtual ~EB_Source();
  protected:
    explicit EB_Source(etherbone::Socket socket_);
    
    static int add_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
    static int get_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
  
    virtual bool prepare(int& timeout);
    virtual bool check();
    virtual bool dispatch(sigc::slot_base* slot);
    
  private:
    etherbone::Socket socket;
    
    typedef std::map<int, Slib::PollFD> fd_map;
    fd_map fds;
};

std::shared_ptr<EB_Source> EB_Source::create(etherbone::Socket socket)
{
  return std::shared_ptr<EB_Source>(new EB_Source(socket));
}

sigc::connection EB_Source::connect(const sigc::slot<bool>& slot)
{
  return connect_generic(slot);
}

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
  
  std::pair<fd_map::iterator, bool> res = 
    self->fds.insert(fd_map::value_type(fd, Slib::PollFD(fd, Slib::IO_ERR)));
  
  if (res.second) // new element; add to poll
    self->add_poll(res.first->second);
  
  Slib::IOCondition flags = Slib::IO_ERR | Slib::IO_HUP;
  if ((mode & EB_DESCRIPTOR_IN)  != 0) flags |= Slib::IO_IN;
  if ((mode & EB_DESCRIPTOR_OUT) != 0) flags |= Slib::IO_OUT;
  
  res.first->second.set_events(flags | res.first->second.get_events());
  return 0;
}

int EB_Source::get_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
{
  EB_Source* self = (EB_Source*)data;
  
  fd_map::iterator i = self->fds.find(fd);
  if (i == self->fds.end()) return 0;
  
  Slib::IOCondition flags = i->second.get_revents();
  
  return 
    ((mode & EB_DESCRIPTOR_IN)  != 0 && (flags & (Slib::IO_IN  | Slib::IO_ERR | Slib::IO_HUP)) != 0) ||
    ((mode & EB_DESCRIPTOR_OUT) != 0 && (flags & (Slib::IO_OUT | Slib::IO_ERR | Slib::IO_HUP)) != 0);
}

static int no_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
{
  return 0;
}

bool EB_Source::prepare(int& timeout_ms)
{
  std::cerr << "EB_Source::prepare" << std::endl;
  // Retrieve cached current time
  int64_t now_ms = get_time();
  
  // Work-around for no TX flow control: flush data now
  socket.check(now_ms/1000, 0, &no_fd);
  
  // Clear the old requested FD status
  for (fd_map::iterator i = fds.begin(); i != fds.end(); ++i)
    i->second.set_events(Slib::IO_ERR);
  
  // Find descriptors we need to watch
  socket.descriptors(this, &EB_Source::add_fd);
  
  // Eliminate any descriptors no one cares about any more
  fd_map::iterator i = fds.begin();
  while (i != fds.end()) {
    fd_map::iterator j = i;
    ++j;
    if ((i->second.get_events() & Slib::IO_HUP) == 0) {
      remove_poll(i->second);
      fds.erase(i);
    }
    i = j;
  }
  
  // Determine timeout
  uint32_t timeout = socket.timeout();
  if (timeout) {
    timeout_ms = static_cast<int64_t>(timeout)*1000 - now_ms;
    if (timeout_ms < 0) timeout_ms = 0;
  } else {
    timeout_ms = -1;
  }
  
  return (timeout_ms == 0); // true means immediately ready
}

bool EB_Source::check()
{
  std::cerr << "EB_Source::check" << std::endl;
  bool ready = false;
  
  // Descriptors ready?
  for (fd_map::const_iterator i = fds.begin(); i != fds.end(); ++i)
    ready |= (i->second.get_revents() & i->second.get_events()) != 0;
  
  // Timeout ready?
  ready |= socket.timeout() >= get_time()/1000;
  
  return ready;
}

bool EB_Source::dispatch(sigc::slot_base* slot)
{
  std::cerr << "EB_Source::dispatch" << std::endl;
  // Process any pending packets
  socket.check(get_time()/1000, this, &EB_Source::get_fd);
  
  // Run the no-op signal
  return (*static_cast<sigc::slot<bool>*>(slot))();
}

static bool my_noop()
{
  return true; // Keep running the loop
}

sigc::connection eb_attach_source(const std::shared_ptr<Slib::MainLoop>& loop, etherbone::Socket socket) {
  std::shared_ptr<EB_Source> source = EB_Source::create(socket);
  sigc::connection out = source->connect(sigc::ptr_fun(&my_noop));
  source->attach(loop->get_context(),source);
  return out;
}
