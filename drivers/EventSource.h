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
#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H

#include "Owned.h"
#include "interfaces/iEventSource.h"

namespace saftlib {

class TimingReceiver;

class EventSource : public Owned, public iEventSource {
  public:
    EventSource(const Glib::ustring& objectPath, TimingReceiver* dev, const Glib::ustring& name, sigc::slot<void> destroy = sigc::slot<void>());
    
    // The name under which this EventSource is listed in TimingReceiver::Iterfaces
    virtual const char *getInterfaceName() const = 0;
    const Glib::ustring &getObjectName() const { return name; }
  
  protected:
    TimingReceiver* dev;
    Glib::ustring name;
};

}

#endif
