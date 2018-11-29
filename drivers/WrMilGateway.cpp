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

#include <assert.h>

#include "RegisteredObject.h"
#include "WrMilGateway.h"
#include "TimingReceiver.h"
#include "wr_mil_gw_regs.h" 
#include "clog.h"

namespace saftlib {

WrMilGateway::WrMilGateway(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev)
{
  // etherbone::Cycle cycle;
  // cycle.open(dev->getDevice());

  // cycle.read()



  // cycle.close();

    // EB_STATUS_OR_VOID_T sdb_find_by_address(eb_address_t address, struct sdb_device* output);
    // EB_STATUS_OR_VOID_T sdb_find_by_identity(uint64_t vendor_id, uint32_t device_id, std::vector<struct sdb_device>& output);
    // EB_STATUS_OR_VOID_T sdb_find_by_identity_msi(uint64_t vendor_id, uint32_t device_id, std::vector<struct sdb_msi_device>& output);
  std::vector<struct sdb_device> devices;
  dev->getDevice().sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), devices);

  std::cerr << "found " << devices.size() << " LM32 devices" << std::endl;


  // dev->getDevice().read(mb_base, EB_DATA32, &mb_value);
  // while((mb_value != 0xffffffff) && slot < 128) {
  //   dev->getDevice().read(mb_base + slot * 4 * 2, EB_DATA32, &mb_value);
  //   slot++;
  // }


  // cycle.read (dev->getBase() + ECA_LATENCY_GET,           EB_DATA32, &raw_latency);
  // cycle.read (dev->getBase() + ECA_OFFSET_BITS_GET,       EB_DATA32, &raw_offset_bits);


  // /* open Etherbone device and socket */
  // if ((eb_status = eb_socket_open(EB_ABI_CODE, 0, EB_ADDR32|EB_DATA32, &socket)) != EB_OK) die(argv[0], "eb_socket_open", eb_status);
  // if ((eb_status = eb_device_open(socket, devName, EB_ADDR32|EB_DATA32, 3, &device)) != EB_OK) die(argv[0], "eb_device_open", eb_status);

  // // find user LM32 devices
  // #define MAX_DEVICES 8
  // struct sdb_device devices[MAX_DEVICES];
  // int num_devices = MAX_DEVICES;
  // eb_sdb_find_by_identity(device, UINT64_C(0x651), UINT32_C(0x54111351), devices, &num_devices);
  // if (num_devices == 0) {
  //   fprintf(stderr, "%s: no matching devices found\n", argv[0]);
  //   return 1;
  // }

  // //printf("found %d devices\n", num_devices);
  // int device_idx = -1;
  // uint64_t device_addr = 0;
  // for (int i = 0; i < num_devices; ++i)
  // {
  //   uint64_t addr_first = devices[i].sdb_component.addr_first;

  //   uint32_t magic_number;
  //   eb_status_t status;
  //   // find the correct user LM32 by reading the first value of the shared memory segment and expecting a certain value for the WR-MIL gateway
  //   status = eb_device_read(device, addr_first+WR_MIL_GW_SHARED_OFFSET, EB_BIG_ENDIAN|EB_DATA32, (eb_data_t*)&magic_number, 0, eb_block);   
  //   if (status != EB_OK){
  //     printf("not ok\n");
  //     continue;
  //   }
  //   else{
  //     //printf("%d %x\n", (int)status, (int)magic_number);
  //   }
    
  //   if (magic_number == WR_MIL_GW_MAGIC_NUMBER)
  //   {
  //     device_idx = 1;
  //     device_addr = addr_first;
  //     break;
  //   }
  // }


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
  return std::vector< guint32 >(10,0);
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
