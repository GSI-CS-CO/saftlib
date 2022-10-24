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

#include "Owned.h"
#include "clog.h"
#include "Device.h"

namespace saftlib {

static void do_unsubscribe(std::shared_ptr<saftbus::Connection> connection, unsigned id) 
{
  connection->signal_unsubscribe(id);
}

Owned::Owned(const std::string& objectPath, sigc::slot<void> destroy_)
 : BaseObject(objectPath), destroy(destroy_)
{
}

Owned::~Owned()
{
  try {
    Destroyed(); 
    if (!owner.empty()) unsubscribe();
  } catch (...) { 
    clog << kLogErr << "Owned::~Owned:: threw an exception!" << std::endl;
  }
}

void Owned::Disown()
{
  if (owner.empty()) {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Do not have an Owner");
  } else {
    ownerOnly();
    unsubscribe();
    owner.clear();
  }
}

void Owned::Own()
{
  initOwner(getConnection(), getSender());
}

void Owned::initOwner(const std::shared_ptr<saftbus::Connection>& connection_, const std::string& owner_)
{
  if (owner.empty()) {
    owner = owner_;
    std::shared_ptr<saftbus::Connection> connection = connection_;
    unsigned subscription_id = connection->signal_subscribe(
        sigc::bind(sigc::ptr_fun(&Owned::owner_quit_handler), this),
        "org.freedesktop.DBus",
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        "/org/freedesktop/DBus",
        owner);
    unsubscribe = sigc::bind(sigc::ptr_fun(&do_unsubscribe), connection, subscription_id);
  } else {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Already have an Owner");
  }
}

void Owned::Destroy()
{
  if (!getDestructible())
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Attempt to Destroy non-Destructible Owned object");
  
  ownerOnly();
  destroy();
}

std::string Owned::getOwner() const
{
  return owner;
}

bool Owned::getDestructible() const
{
  return !destroy.empty();
}

void Owned::ownerOnly() const
{
  if (!owner.empty() && owner != getSender())
    throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "You are not my Owner");
}

void Owned::ownerQuit()
{
}

void Owned::owner_quit_handler(
  const std::shared_ptr<saftbus::Connection>&,
  const std::string&, const std::string&, const std::string&,
  const std::string&, const saftbus::Serial&,
  Owned* self)
{
  try {
    self->unsubscribe();
    self->owner.clear();
    self->ownerQuit(); // inform base classes, in case they have clean-up code
    
    if (self->getDestructible()) self->destroy();
    // do not use self beyond this point
  } catch (const etherbone::exception_t& e) {
    clog << kLogErr << "Owned::owner_quit_handler: " << e << std::endl; 
  } catch (const saftbus::Error& e) {           
    clog << kLogErr << "Owned::owner_quit_handler: " << e.what() << std::endl; 
  } catch (...) {
    clog << kLogErr << "Owned::owner_quit_handler: unknown exception" << std::endl;
  }
}

}
