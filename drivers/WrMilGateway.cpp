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
#include <iomanip>

#include <assert.h>

#include "RegisteredObject.h"
#include "WrMilGateway.h"
#include "TimingReceiver.h"
#include "wr_mil_gw_regs.h" 
#include "clog.h"

namespace saftlib {

WrMilGateway::WrMilGateway(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   have_wrmilgw(false),
   registerContent(13,0)
{
  // get all LM32 devices and see if any of them has the WR-MIL-Gateway magic number
  std::vector<struct sdb_device> devices;
  dev->getDevice().sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), devices);
  for(auto device: devices) {
    guint32 wr_mil_gw_magic_number = 0;
    dev->getDevice().read(device.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET, 
                          EB_DATA32, 
                          (eb_data_t*)&wr_mil_gw_magic_number);
    if (wr_mil_gw_magic_number == WR_MIL_GW_MAGIC_NUMBER) {
      wrmilgw_device = device;
      have_wrmilgw = true;
      break;
    }
  }

  // just throw if there is no active firmware
  if (!have_wrmilgw) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "No WR-MIL-Gateway found");
  }

   // see if the firmware is running (it should reset the CMD register to 0 after a command is put there)
   // submit a test command 
  dev->getDevice().write(wrmilgw_device.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET + WR_MIL_GW_REG_COMMAND,
                         EB_DATA32,
                         (eb_data_t)WR_MIL_GW_CMD_TEST);
  usleep(50000);
  guint32 value;
  dev->getDevice().read(wrmilgw_device.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET + WR_MIL_GW_REG_COMMAND,
                         EB_DATA32,
                         (eb_data_t*)&value);
  // the firmware is not running if the command value was not overwritten
  if (value) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "WR-MIL-Gateway not running");
  }

  // read all registers once
  getRegisterContent();
}

WrMilGateway::~WrMilGateway()
{
}

Glib::RefPtr<WrMilGateway> WrMilGateway::create(const ConstructorType& args)
{
  return RegisteredObject<WrMilGateway>::create(args.objectPath, args);
}

std::vector< guint32 > WrMilGateway::getRegisterContent() const
{
  for (unsigned i = 0; i < registerContent.size(); ++i) {
    dev->getDevice().read(wrmilgw_device.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET + 4*i, 
                          EB_DATA32, 
                          (eb_data_t*)&registerContent[i]);
  }

  return registerContent;
}

void WrMilGateway::Reset() 
{
}

void WrMilGateway::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

}
