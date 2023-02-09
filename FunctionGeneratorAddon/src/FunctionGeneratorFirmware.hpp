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
 
#ifndef FUNCTION_GENERATOR_FIRMWARE_HPP_
#define FUNCTION_GENERATOR_FIRMWARE_HPP_

#include "Owned.hpp"
#include <TimingReceiver.hpp>
#include "FunctionGenerator.hpp"

namespace saftlib {

class TimingReceiver;


/// @brief Representation of the FunctionGenerator Firmware
/// 
/// It can trigger the firmware to do a channel rescan.
/// The channels can be assigned to individual FunctionGenerator objects or to a single MasterFunctionGenerator object
class FunctionGeneratorFirmware : public Owned, public TimingReceiverAddon
{
	
  public:

    std::map< std::string , std::map<std::string, std::string> > getObjects(); // TimingReceiverAddon pure-virtual override
    std::string getObjectPath(); // used in create_service.cpp


    FunctionGeneratorFirmware(saftbus::Container *container, SAFTd *saft_daemon, TimingReceiver *timing_receiver);
    ~FunctionGeneratorFirmware();
    // typedef FunctionGeneratorFirmware_Service ServiceType;
    // struct ConstructorType {
    //   std::string objectPath;
    //   TimingReceiver  *tr;
    //   Device &device;
    //   etherbone::sdb_msi_device  sdb_msi_base;
    //   sdb_device                 mailbox;
    //   std::map< std::string, std::shared_ptr<Owned> > &fgs_owned;
    //   std::map< std::string, std::shared_ptr<Owned> > &master_fgs_owned;
    // };
    // static std::shared_ptr<FunctionGeneratorFirmware> create(const ConstructorType& args);
    
    uint32_t getVersion() const;

    /// @brief Scan bus for fg channels. 
    ///
    /// This function should only be called if no fg-channel is active.
    /// If any channel is active and this function is called a saftbus::Error will be thrown!
    /// @return   fgs found.
    ///
    // @saftbus-export
    std::map<std::string, std::string> Scan();
    // @saftbus-export
    std::map<std::string, std::string> ScanMasterFg();
    // @saftbus-export
    std::map<std::string, std::string> ScanFgChannels();

    void removeMasterFunctionGenerator();
    
  protected:

    void firmware_rescan(int host_to_lm32_mailbox_slot_idx);

    bool nothing_runs();

    saftbus::Container         *container;

    std::string                objectPath;
    SAFTd                      *saftd;
    TimingReceiver             *tr;
    Mailbox                    *mbox;
    etherbone::Device&         device;
    // etherbone::sdb_msi_device  sdb_msi_base;
    // sdb_device                 mailbox;

    std::map< std::string, std::shared_ptr<FunctionGenerator> > fgs;
    // std::map< std::string, std::shared_ptr<Owned> > &master_fgs_owned;

    bool have_fg_firmware;
    eb_data_t magic;
    eb_data_t version;
    eb_address_t fgb;
  
    std::map<std::string, std::map<std::string, std::string> > addon_objects;


};

}

#endif
