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
#ifndef SAFTLIB_DEVICES_H
#define SAFTLIB_DEVICES_H

#include <etherbone.h>
#include <giomm.h>

namespace saftlib {

// Saftlib devices just add IRQs
class Device : public etherbone::Device {
  public:
    Device(etherbone::Device d);
    
    eb_address_t request_irq(const sigc::slot<void,eb_data_t>& slot);
    void release_irq(eb_address_t);
    
    static void hook_it_all(etherbone::Socket s);
    static sigc::connection attach(const Glib::RefPtr<Glib::MainLoop>& loop);
    
  private:
    eb_address_t low, high;
    
    typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
    static irqMap irqs;
    
    struct MSI { eb_address_t address, data; };
    typedef std::deque<MSI> msiQueue;
    static msiQueue msis;

  friend class IRQ_Handler;
  friend class MSI_Source;
};

}

#endif
