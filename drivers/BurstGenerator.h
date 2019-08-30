
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
      int32_t instruct(uint32_t code, const std::vector< uint32_t >& args);
      std::vector< uint32_t > readBurstInfo(uint32_t id);
      std::vector< uint32_t > readSharedBuffer(uint32_t size);
      uint32_t getResponse() const;

    protected:
      BurstGenerator(const ConstructorType& args);
      ~BurstGenerator();
      bool firmwareRunning(uint32_t id);
      void msi_handler(eb_data_t msg);

      std::string                objectPath;
      TimingReceiver             *tr;
      Device&                    device;
      etherbone::sdb_msi_device  sdb_msi_base;
      sdb_device                 mailbox;
      eb_address_t               my_msi_path;   // my msi path (required to configure mailbox)
      int                        my_slot;       // mailbox slot subscribed by me
      uint32_t                   inst_code;     // instruction code sent via MSI

      bool                       found_bg_fw;
      eb_address_t               bg_slot;       // mailbox slot subscribed by the burst generator
      eb_address_t               ram_base;

  };

}

#endif
