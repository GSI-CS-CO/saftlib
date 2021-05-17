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
#ifndef SAFTLIB_DEVICES_H
#define SAFTLIB_DEVICES_H

#include <deque>
#include <memory>
#include <etherbone.h>
#include <sigc++/sigc++.h>
#include <boost/circular_buffer.hpp>
#include "MainLoop.h"

namespace saftlib {

// Saftlib devices just add IRQs
class Device : public etherbone::Device {
  public:
    Device(etherbone::Device d, eb_address_t first, eb_address_t last, bool poll = false);
    
    eb_address_t request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot);
    void release_irq(eb_address_t);
    
    static void hook_it_all(etherbone::Socket s);
    static sigc::connection attach(const std::shared_ptr<Slib::MainLoop>& loop);

    static void set_msi_buffer_capacity(size_t capacity);
    
  private:
    eb_address_t base;
    eb_address_t mask;
    
    typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
    static irqMap irqs;

    struct MSI { eb_address_t address, data; };
    typedef boost::circular_buffer<MSI> msiQueue;
    static msiQueue msis;
    
    bool activate_msi_polling;
    bool poll_msi();
    eb_address_t msi_first;

  friend class IRQ_Handler;
  friend class MSI_Source;
};

} // namespace saftlib

#endif
