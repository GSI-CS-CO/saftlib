/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "EventSource.h"
#include "clog.h"

namespace saftlib {

EventSource::EventSource(const Glib::ustring& objectPath, TimingReceiver* dev_, const Glib::ustring& name_, sigc::slot<void> destroy)
 : Owned(objectPath, destroy), dev(dev_), name(name_)
{
}

guint64 EventSource::getResolution() const
{
  return 0; // !!!
}

guint32 EventSource::getEventBits() const
{
  return 0; // !!!
}

bool EventSource::getEventEnable() const
{
  return false; // !!!
}

guint64 EventSource::getEventPrefix() const
{
  return 0; // !!!
}
    
void EventSource::setEventEnable(bool val)
{
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void EventSource::setEventPrefix(guint64 val)
{
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

}
