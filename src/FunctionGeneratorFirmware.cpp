/*  Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include <iostream>
#include <memory>
#include <assert.h>

#include <TimingReceiver.h>
#include <fg_regs.h>

#include "FunctionGeneratorFirmware.hpp"

#include "FunctionGeneratorImpl.hpp"

#include "FunctionGenerator_Service.hpp"
#include "MasterFunctionGenerator_Service.hpp"
// #include "RegisteredObject.h"
// #include "FunctionGeneratorImpl.h"
// #include "FunctionGenerator.h"
// #include "MasterFunctionGenerator.h"

namespace saftlib {

FunctionGeneratorFirmware::FunctionGeneratorFirmware(saftbus::Container *cont, SAFTd *saft_daemon, TimingReceiver *timing_receiver)
 : Owned(cont)
 , container(cont)
 , objectPath(timing_receiver->getObjectPath()+"/fg_firmware")
 , saftd(saft_daemon)
 , tr(timing_receiver)
 , mbox(static_cast<Mailbox*>(timing_receiver))
 , device(tr->OpenDevice::get_device())
{
    etherbone::Cycle cycle;
    
    // Probe for LM32 block memories
    fgb = 0;

    // Check them all for the function generator microcontroller
    have_fg_firmware = false;
    for (unsigned i = 0; i < tr->LM32Cluster::getCpuCount(); ++i) {
      fgb = timing_receiver->LM32Cluster::dpram_lm32_adr_first[i];
      
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
}

FunctionGeneratorFirmware::~FunctionGeneratorFirmware()
{
  // std::cerr << "~FunctionGeneratorFirmware()" << std::endl;

  clear();
}

void FunctionGeneratorFirmware::clear()
{
  // std::cerr << "FunctionGeneratorFirmware::clear()" << std::endl;
  try {
    std::vector<std::string> fg_names;
    for (auto &fg: fgs) if (fg.second) fg_names.push_back(fg.second->getObjectPath());
    std::string mfg_name;  
    if (mfg) mfg_name = mfg->getObjectPath();
    fgs.clear();
    mfg.reset();
    addon_objects.clear();
    if (container) {
      for (auto &fg: fg_names) {
        container->remove_object(fg);
      }
      if (mfg_name.size()) container->remove_object(mfg_name);
    }
  } catch (...) { // catch exceptions that occur when the object was removed before
    // nothing.
  }
}

std::string FunctionGeneratorFirmware::getObjectPath() 
{
  return objectPath;
}

std::map< std::string , std::map<std::string, std::string> > FunctionGeneratorFirmware::getObjects() {
  std::map< std::string, std::map<std::string, std::string> > result;
  result["FunctionGeneratorFirmware"]["fg_firmware"] = objectPath;
  if (addon_objects["FunctionGenerator"].size()) {
    result["FunctionGenerator"] = addon_objects["FunctionGenerator"];
  }
  result["MasterFunctionGenerator"] = addon_objects["MasterFunctionGenerator"];
  return result;
}


uint32_t FunctionGeneratorFirmware::getVersion() const
{
  // DRIVER_LOG("",-1,-1);
  return static_cast<uint32_t>(version);
}

bool FunctionGeneratorFirmware::nothing_runs()
{
  // DRIVER_LOG("",-1,-1);
  for (auto &fg: fgs) {
    if (fg.second->getRunning()) {
      return false;
    }
  }



  if (mfg) {
    std::vector<int> running = mfg->ReadRunning();
    for (auto runs: running) {
      if (runs) {
        return false;
      }
    }

  }

  return true;
}


std::map<std::string, std::string> FunctionGeneratorFirmware::Scan()
{
  // DRIVER_LOG("",-1,-1);
  return ScanMasterFg();
  // return ScanFgChannels();
}

void FunctionGeneratorFirmware::firmware_rescan(int host_to_lm32_mailbox_slot_idx)
{
  // DRIVER_LOG("",-1,-1);
  // initiate firmware rescan and wait until firmware is done
  if (version == 3) device.write(fgb + SHM_BASE + FG_SCAN_DONE, EB_DATA32, 1); 
  if (version == 4) device.write(fgb + SHM_BASE + FG_BUSY, EB_DATA32, 1);

  mbox->UseSlot(host_to_lm32_mailbox_slot_idx, SWI_SCAN);
  // device.write(swi, EB_DATA32, SWI_SCAN);
  // wait until firmware is done scanning
  for (int i = 0;; ++i) {
    eb_data_t done; 
    if (version == 3) device.read(fgb + SHM_BASE + FG_SCAN_DONE, EB_DATA32, &done);
    if (version == 4) device.read(fgb + SHM_BASE + FG_BUSY, EB_DATA32, &done);
    if (done==0) {
      break;
    }
    if (i == 200) { // wait at least 20 seconds before timeout
      // DRIVER_LOG("hit_20s_timeout",-1,-1);
      throw saftbus::Error(saftbus::Error(saftbus::Error::FAILED,"timeout while waiting for fg-firmware scan"));
    }
    usleep(100000); // 100 ms 
  }
}


// pass sigc signals from impl class to dbus
// to reduce traffic only generate signals if we have an owner
std::map<std::string, std::string> FunctionGeneratorFirmware::ScanMasterFg()
{
  // DRIVER_LOG("",-1,-1);
  ownerOnly();
  if (!nothing_runs()) {
    throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "FunctionGeneratorFirmware::Scan is not allowed if any channel is active");
  }

  // fgs_owned.clear();
  // master_fgs_owned.clear();

  clear();

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
    // eb_address_t swi = mailbox.sdb_component.addr_first + mb_slot * 4 * 2;
    std::cerr << "mailbox slot for swi is 0x" << std::hex << mb_slot << std::endl;
    eb_data_t num_channels, buffer_size, macros[FG_MACROS_SIZE];
    
    // firmware scan and wait until it is done
    firmware_rescan(mb_slot);

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
    for (unsigned j = 0; j < num_channels; ++j) {
      // cycle.write(swi, EB_DATA32, SWI_DISABLE | j);
      mbox->UseSlot(mb_slot, SWI_DISABLE | j);
    }
    cycle.close();

    std::vector<std::shared_ptr<FunctionGeneratorImpl> > functionGeneratorImplementations;           
    // fgs_owned.clear();
    // master_fgs_owned.clear();
    // Create the objects to control the channels
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
      if (!macros[j]) {
        continue; // no hardware
      }


      // std::ostringstream spath;
      // // spath.imbue(std::locale("C"));
      // spath << objectPath << "/fg_" << j;
      // std::string path = spath.str();
      
      functionGeneratorImplementations.push_back(std::make_shared<FunctionGeneratorImpl>(saftd,tr,allocation,fgb,mb_slot,num_channels,buffer_size,j,macros[j]));

      // std::ostringstream spath;
      // spath.imbue(std::locale("C"));
      // spath << objectPath << "/fg_" << j;
      // std::string path = spath.str();
      // FunctionGeneratorImpl::ConstructorType args = { path, tr, allocation, fgb, swi, sdb_msi_base, mailbox, (unsigned)num_channels, (unsigned)buffer_size, j, (uint32_t)macros[j] };
      // functionGeneratorImplementations.push_back(std::make_shared<FunctionGeneratorImpl>(args));
    }

    std::string name = "masterfg";
    std::ostringstream mfg_spath;
    // mfg_spath.imbue(std::locale("C"));
    mfg_spath << objectPath << "/" << name ;
    std::string mfg_path = mfg_spath.str();


    mfg = std::make_shared<MasterFunctionGenerator>(container, mfg_path, functionGeneratorImplementations);

    if (container) {
      std::unique_ptr<MasterFunctionGenerator_Service> service(new MasterFunctionGenerator_Service(mfg.get(), std::bind(&FunctionGeneratorFirmware::clear, this) ));
      mfg->set_service(service.get());
      container->create_object(mfg_path, std::move(service));
    }

    result.insert(std::make_pair(name, mfg_path));

 
  }
  addon_objects["MasterFunctionGenerator"] = result;

  return result;
}


std::map<std::string, std::string> FunctionGeneratorFirmware::ScanFgChannels()
{

  // std::cerr << "FunctionGeneratorFirmware::ScanFgChannels() getDestructible() " << getDestructible() << std::endl;
  // DRIVER_LOG("",-1,-1);
  ownerOnly();
  if (!nothing_runs()) {
    throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "FunctionGeneratorFirmware::Scan is not allowed if any channel is active");
  }

  // if (container) {
  //   for(auto &fg: fgs) {
  //     container->remove_object(fg.second->getObjectPath());
  //   }
  //   container->remove_object(mfg->getObjectPath());
  // }

  // fgs.clear();
  // // master_fgs_owned.clear();

  clear();

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
    // eb_address_t swi = mailbox.sdb_component.addr_first + mb_slot * 4 * 2;
    std::cerr << "ScanFgChannels: mailbox slot for swi is 0x" << mb_slot << std::endl;
    eb_data_t num_channels, buffer_size, macros[FG_MACROS_SIZE];
    
    // firmware scan and wait until it is done
    firmware_rescan(mb_slot);

    // Probe the configuration and hardware macros
    cycle.open(device);
    cycle.read(fgb + SHM_BASE + FG_NUM_CHANNELS, EB_DATA32, &num_channels);
    cycle.read(fgb + SHM_BASE + FG_BUFFER_SIZE,  EB_DATA32, &buffer_size);
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j)
      cycle.read(fgb + SHM_BASE + FG_MACROS + j*4, EB_DATA32, &macros[j]);
    cycle.close();
    std::cerr << "ScanFgChannels: num_channels " << num_channels << std::endl;
    
    // Create an allocation buffer
    std::shared_ptr<std::vector<int>> allocation(
      new std::vector<int>);
    allocation->resize(num_channels, -1);
    
    // Disable all channels
    cycle.open(device);
    for (unsigned j = 0; j < num_channels; ++j) {
      // cycle.write(swi, EB_DATA32, SWI_DISABLE | j);
      mbox->UseSlot(mb_slot, SWI_DISABLE | j);
    }
    cycle.close();

    // std::vector<std::shared_ptr<FunctionGeneratorImpl> > functionGeneratorImplementations;           
    // fgs_owned.clear();
    // master_fgs_owned.clear();
    // Create the objects to control the channels
    for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
            if (!macros[j]) {
              continue; // no hardware
            }
      
      std::ostringstream spath;
      // spath.imbue(std::locale("C"));
      spath << objectPath << "/fg_" << j;
      std::string path = spath.str();
      
      std::shared_ptr<FunctionGeneratorImpl> fg_impl = std::make_shared<FunctionGeneratorImpl>(saftd,tr,allocation,fgb,mb_slot,num_channels,buffer_size,j,macros[j]);
      // FunctionGeneratorImpl::ConstructorType args = { path, tr, allocation, fgb, swi, sdb_msi_base, mailbox, (unsigned)num_channels, (unsigned)buffer_size, j, (uint32_t)macros[j] };
      // FunctionGenerator::ConstructorType fgargs = { path, tr, std::make_shared<FunctionGeneratorImpl>(args) };
      // std::shared_ptr<FunctionGenerator> fg = FunctionGenerator::create(fgargs);

      std::ostringstream name;
      name << "fg-" << (int)fg_impl->getSCUbusSlot() 
           << "-"   << (int)fg_impl->getDeviceNumber();

      std::shared_ptr<FunctionGenerator> fg = std::make_shared<FunctionGenerator>(container, name.str(), path, fg_impl);

      if (container) {
        std::unique_ptr<FunctionGenerator_Service> service(new FunctionGenerator_Service(fg.get()));
        fg->set_service(service.get());
        fg->Own();
        container->create_object(path, std::move(service));
      }
      fgs[name.str()] = fg;

      result.insert(std::make_pair(name.str(), path));
    }

  }
  addon_objects["FunctionGenerator"] = result;
  return result;
}



}
