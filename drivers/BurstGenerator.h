
#ifndef BURST_GENERATOR_H
#define BURST_GENERATOR_FIRMWARE_H

#include <deque>

#include "interfaces/BurstGenerator.h" // BurstGenerator_Service/Proxy classes
#include "Owned.h"

namespace saftlib {

  class TimingReceiver;

  class BurstGenerator : public Owned, public iBurstGenerator
  {

    public:
      typedef BurstGenerator_Service ServiceType; // Service interface
      struct ConstructorType {                    // type for constructor
        std::string                objectPath;
        TimingReceiver             *tr;
        Device                     &device;
        etherbone::sdb_msi_device  sdb_msi_base;
        sdb_device                 mailbox;
      };

      static std::shared_ptr<BurstGenerator> create(const ConstructorType& args);

      // iBurstGenerator overrides
      uint32_t readFirmwareId();
      int32_t sendPulseParams(uint32_t eventHi, uint32_t delay, uint32_t conditions, uint32_t offset);
      int32_t sendProdCycles(uint32_t eventHi, uint64_t cycles);
      int32_t instruct(uint32_t code, const std::vector< uint32_t >& args);
      std::vector< uint32_t > readBurstInfo(uint32_t id);

    protected:
      BurstGenerator(const ConstructorType& args);
      ~BurstGenerator();
      bool detectFirmware(std::vector<sdb_device> ram, uint32_t id, eb_address_t& ram_base);

      std::string                objectPath;
      TimingReceiver             *tr;
      Device&                    device;
      etherbone::sdb_msi_device  sdb_msi_base;
      sdb_device                 mailbox;

      bool                       found_bg_fw;
      eb_address_t               mb_slot;
      eb_address_t               ram_base;

  };

}

#endif
