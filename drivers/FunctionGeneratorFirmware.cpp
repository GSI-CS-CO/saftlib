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
#include "FunctionGeneratorFirmware.h"
#include "FunctionGeneratorImpl.h"
#include "FunctionGenerator.h"
#include "MasterFunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"

namespace saftlib {

FunctionGeneratorFirmware::FunctionGeneratorFirmware(const ConstructorType& args)
 : Owned(args.objectPath),
   objectPath(args.objectPath),
   tr(args.tr),
   device(args.device),
   sdb_msi_base(args.sdb_msi_base),
   mailbox(args.mailbox),
   fgs_owned(args.fgs_owned),
   master_fgs_owned(args.master_fgs_owned),
   have_fg_firmware(false)
{
    etherbone::Cycle cycle;
    
    // Probe for LM32 block memories
    fgb = 0;
    std::vector<sdb_device> fgs, rom;
    device.sdb_find_by_identity(LM32_RAM_USER_VENDOR,    LM32_RAM_USER_PRODUCT,    fgs);
    device.sdb_find_by_identity(LM32_CLUSTER_ROM_VENDOR, LM32_CLUSTER_ROM_PRODUCT, rom);
    
    
    if (rom.size() != 1)
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "SCU is missing LM32 cluster ROM");
    
    eb_address_t rom_address = rom[0].sdb_component.addr_first;
    eb_data_t cpus, eps_per;
    cycle.open(device);
    cycle.read(rom_address + 0x0, EB_DATA32, &cpus);
    cycle.read(rom_address + 0x4, EB_DATA32, &eps_per);
    cycle.close();
    
    if (cpus != fgs.size())
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Number of LM32 RAMs does not equal ROM cpu_count");
    
    // Check them all for the function generator microcontroller
    for (unsigned i = 0; i < fgs.size(); ++i) {
      fgb = fgs[i].sdb_component.addr_first;
      
      cycle.open(device);
      cycle.read(fgb + SHM_BASE + FG_MAGIC_NUMBER, EB_DATA32, &magic);
      cycle.read(fgb + SHM_BASE + FG_VERSION,      EB_DATA32, &version);
      cycle.close();
      if (magic == 0xdeadbeef && (version == 3 || version == 4)) 
      {
        have_fg_firmware = true;
        break;
      }
    }

    if (!have_fg_firmware) throw saftbus::Error(saftbus::Error::FAILED, "no FunctionGeneratorFirmware found");

    try {
      Scan(); // do the initial scan. If there's a timeout here, the 
              //  FunctionGeneratorFirmware driver should still be loaded 
              //  that's why this is in a try catch block
    } catch (saftbus::Error &e) {
      clog << kLogDebug << "Scan FG-channels timeout" << std::endl;
    }
}

FunctionGeneratorFirmware::~FunctionGeneratorFirmware()
{
}

std::shared_ptr<FunctionGeneratorFirmware> FunctionGeneratorFirmware::create(const ConstructorType& args)
{
  return RegisteredObject<FunctionGeneratorFirmware>::create(args.objectPath, args);
}

uint32_t FunctionGeneratorFirmware::getVersion() const
{
  return static_cast<uint32_t>(version);
}

bool FunctionGeneratorFirmware::nothing_runs()
{
  for (auto fgs: fgs_owned) {
    auto fg = std::dynamic_pointer_cast<FunctionGenerator>(fgs.second);
    if (fg->getRunning()) {
      return false;
    }
  }

  for (auto mfgs: master_fgs_owned) {
    auto mfg = std::dynamic_pointer_cast<MasterFunctionGenerator>(mfgs.second);
    for (auto running: mfg->ReadRunning()) {
      if (running) {
        return false;
      }
    }
  }

  return true;
}


std::map<std::string, std::string> FunctionGeneratorFirmware::Scan()
{
  return ScanMasterFg();
}

void FunctionGeneratorFirmware::firmware_rescan(eb_address_t swi)
{
  // initiate firmware rescan and wait until firmware is done
  if (version == 3) tr->getDevice().write(fgb + SHM_BASE + FG_SCAN_DONE, EB_DATA32, 1); 
  if (version == 4) tr->getDevice().write(fgb + SHM_BASE + FG_BUSY, EB_DATA32, 1);
  tr->getDevice().write(swi, EB_DATA32, SWI_SCAN);
  // wait until firmware is done scanning
  for (int i = 0;; ++i) {
    eb_data_t done; 
    if (version == 3) tr->getDevice().read(fgb + SHM_BASE + FG_SCAN_DONE, EB_DATA32, &done);
    if (version == 4) tr->getDevice().read(fgb + SHM_BASE + FG_BUSY, EB_DATA32, &done);
    if (done==0) {
      break;
    }
    if (i == 200) { // wait at least 20 seconds before timeout
      throw saftbus::Error(saftbus::Error(saftbus::Error::FAILED,"timeout while waiting for fg-firmware scan"));
    }
    usleep(100000); // 100 ms 
  }
}


// pass sigc signals from impl class to dbus
// to reduce traffic only generate signals if we have an owner
std::map<std::string, std::string> FunctionGeneratorFirmware::ScanMasterFg()
{
  ownerOnly();
  if (!nothing_runs()) {
    throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "FunctionGeneratorFirmware::Scan is not allowed if any channel is active");
  }

  fgs_owned.clear();
  master_fgs_owned.clear();

  std::map<std::string, std::string> result;
  if (have_fg_firmware) {

    etherbone::Cycle cycle;

    // get mailbox slot number for swi host=>lm32
    eb_data_t mb_slot;
    cycle.open(device);
    cycle.read(fgb + SHM_BASE + FG_MB_SLOT, EB_DATA32, &mb_slot);
    cycle.close(); 

    if (mb_slot < 0 && mb_slot > 127)
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "mailbox slot number not in range 0 to 127");

    // swi address of fg is to be found in mailbox slot mb_slot
    eb_address_t swi = mailbox.sdb_component.addr_first + mb_slot * 4 * 2;
    clog << kLogDebug << "mailbox address for swi is 0x" << std::hex << swi << std::endl;
    eb_data_t num_channels, buffer_size, macros[FG_MACROS_SIZE];
    
    // firmware scan and wait until it is done
    firmware_rescan(swi);

    // Probe the configuration and hardware macros
    cycle.open(device);
    cycle.read(fgb + SHM_BASE + FG_NUM_CHANNELS, EB_DATA32, &num_channels);
    cycle.read(fgb + SHM_BASE + FG_BUFFER_SIZE,  EB_DATA32, &buffer_size);
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j)
      cycle.read(fgb + SHM_BASE + FG_MACROS + j*4, EB_DATA32, &macros[j]);
    cycle.close();
    
    // Create an allocation buffer
    std::shared_ptr<std::vector<int>> allocation(
      new std::vector<int>);
    allocation->resize(num_channels, -1);
    
    // Disable all channels
    cycle.open(device);
    for (unsigned j = 0; j < num_channels; ++j)
      cycle.write(swi, EB_DATA32, SWI_DISABLE | j);
    cycle.close();

    std::vector<std::shared_ptr<FunctionGeneratorImpl> > functionGeneratorImplementations;           
    fgs_owned.clear();
    master_fgs_owned.clear();
    // Create the objects to control the channels
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
      if (!macros[j]) {
        continue; // no hardware
      }
      std::ostringstream spath;
      spath.imbue(std::locale("C"));
      spath << objectPath << "/fg_" << j;
      std::string path = spath.str();
      FunctionGeneratorImpl::ConstructorType args = { path, tr, allocation, fgb, swi, sdb_msi_base, mailbox, (unsigned)num_channels, (unsigned)buffer_size, j, (uint32_t)macros[j] };
      functionGeneratorImplementations.push_back(std::make_shared<FunctionGeneratorImpl>(args));
    }

    std::ostringstream mfg_spath;
    mfg_spath.imbue(std::locale("C"));
    mfg_spath << objectPath << "/masterfg";
    std::string mfg_path = mfg_spath.str();

    MasterFunctionGenerator::ConstructorType mfg_args = { mfg_path, tr, functionGeneratorImplementations};
    std::shared_ptr<MasterFunctionGenerator> mfg = MasterFunctionGenerator::create(mfg_args);

    master_fgs_owned["masterfg"] = mfg;
    result.insert(std::make_pair("masterfg", mfg_path));

  }

  return result;
}


std::map<std::string, std::string> FunctionGeneratorFirmware::ScanFgChannels()
{
  ownerOnly();
  if (!nothing_runs()) {
    throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "FunctionGeneratorFirmware::Scan is not allowed if any channel is active");
  }

  fgs_owned.clear();
  master_fgs_owned.clear();

  std::map<std::string, std::string> result;
  if (have_fg_firmware) {

    etherbone::Cycle cycle;

    // get mailbox slot number for swi host=>lm32
    eb_data_t mb_slot;
    cycle.open(device);
    cycle.read(fgb + SHM_BASE + FG_MB_SLOT, EB_DATA32, &mb_slot);
    cycle.close(); 

    if (mb_slot < 0 && mb_slot > 127)
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "mailbox slot number not in range 0 to 127");

    // swi address of fg is to be found in mailbox slot mb_slot
    eb_address_t swi = mailbox.sdb_component.addr_first + mb_slot * 4 * 2;
    clog << kLogDebug << "mailbox address for swi is 0x" << std::hex << swi << std::endl;
    eb_data_t num_channels, buffer_size, macros[FG_MACROS_SIZE];
    
    // firmware scan and wait until it is done
    firmware_rescan(swi);

    // Probe the configuration and hardware macros
    cycle.open(device);
    cycle.read(fgb + SHM_BASE + FG_NUM_CHANNELS, EB_DATA32, &num_channels);
    cycle.read(fgb + SHM_BASE + FG_BUFFER_SIZE,  EB_DATA32, &buffer_size);
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j)
      cycle.read(fgb + SHM_BASE + FG_MACROS + j*4, EB_DATA32, &macros[j]);
    cycle.close();
    
    // Create an allocation buffer
    std::shared_ptr<std::vector<int>> allocation(
      new std::vector<int>);
    allocation->resize(num_channels, -1);
    
    // Disable all channels
    cycle.open(device);
    for (unsigned j = 0; j < num_channels; ++j)
      cycle.write(swi, EB_DATA32, SWI_DISABLE | j);
    cycle.close();

    std::vector<std::shared_ptr<FunctionGeneratorImpl> > functionGeneratorImplementations;           
    fgs_owned.clear();
    master_fgs_owned.clear();
    // Create the objects to control the channels
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
            if (!macros[j]) {
              continue; // no hardware
            }
      
      std::ostringstream spath;
      spath.imbue(std::locale("C"));
      spath << objectPath << "/fg_" << j;
      std::string path = spath.str();
      
      FunctionGeneratorImpl::ConstructorType args = { path, tr, allocation, fgb, swi, sdb_msi_base, mailbox, (unsigned)num_channels, (unsigned)buffer_size, j, (uint32_t)macros[j] };
      FunctionGenerator::ConstructorType fgargs = { path, tr, std::make_shared<FunctionGeneratorImpl>(args) };
      std::shared_ptr<FunctionGenerator> fg = FunctionGenerator::create(fgargs);       
      std::ostringstream name;
      name.imbue(std::locale("C"));
      name << "fg-" << (int)fgargs.functionGeneratorImpl->getSCUbusSlot() 
           << "-"   << (int)fgargs.functionGeneratorImpl->getDeviceNumber();
      fgs_owned[name.str()] = fg;
      result.insert(std::make_pair(name.str(), path));
    }
  }

  return result;
}



}
