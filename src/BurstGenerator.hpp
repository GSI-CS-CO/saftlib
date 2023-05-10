
#ifndef BURST_GENERATOR_FIRMWARE_HPP_
#define BURST_GENERATOR_FIRMWARE_HPP_

#include "Owned.hpp"
#include <TimingReceiver.hpp>
#include <TimingReceiverAddon.hpp>
#include <Mailbox.hpp>

#include <saftbus/service.hpp> // for saftbus::Container

#include <sigc++/sigc++.h>

#include <memory>

namespace saftlib {

  ///  @brief Burst generation service
  ///
  ///  A dedicated firmware must be loaded and running in the embedded LM32 core
  ///  prior to the burst generation. The methods and signals of this interface
  ///  are responsible to inform about the underlying hardware, firmware and
  ///  load the firmware binary if necessary.
  class BurstGenerator : public Owned , public TimingReceiverAddon
  {

    public:
      std::map<std::string, std::map<std::string, std::string> > getObjects(); // TimingReceiverAddon pure-virtual override
      std::string getObjectPath(); // used in create_service.cpp

      BurstGenerator(saftbus::Container *container, SAFTd *saft_daemon, TimingReceiver *timing_receiver);
      ~BurstGenerator();


      /// @brief Instruction to the burst generator.
      ///
      /// It is a common method to communicate with the burst generator.
      /// The method contains an instruction code and corresponding arguments.
      /// @param code   User instruction code for LM32
      /// @param args   Instruction arguments (vector of u32 integers)
      /// @param result Return result. 0 on success.
      ///  
      // @saftbus-export
      int32_t instruct(uint32_t code, const std::vector< uint32_t >& args);

      /// @brief Get burst info.
      ///
      /// The info includes the type and index of IO port, IDs of trigger and toggling events.
      /// @param id     The burst ID
      /// @param info   The burst info (vector of u32 integers)
      ///
      // @saftbus-export
      std::vector< uint32_t > readBurstInfo(uint32_t id);

      /// @brief Read the shared memory.
      ///
      /// @param size   The amount of data to be read
      /// @param data   The read data
      ///
      // @saftbus-export
      std::vector< uint32_t > readSharedBuffer(uint32_t size);


      /// @brief Read the actual state of the burst generator.
      /// @param state   Actual state of the burst generator
      ///
      // @saftbus-export
      uint32_t readState();


      /// @brief The burst generator response sent via mailbox.
      /// @param code User instruction code
      // @saftbus-export
      uint32_t getResponse() const;

      /// @brief Notify the completion of an instruction.
      ///
      /// It is the response of the burst generator on user instruction request.
      /// @param code   User instruction code
      ///
      // @saftbus-export
      sigc::signal<void, uint32_t> sigInstComplete;

    protected:
      bool firmwareRunning(uint32_t id);
      void msi_handler(eb_data_t msg);

      std::string                objectPath;
      SAFTd                      *saftd;        // in saftlib-v3 SAFTd is needed for request_irq and release_irq
      TimingReceiver             *tr;           // TimingReceiver provides all needed SdbDevices
      etherbone::Device&         device;        // a reference to the etherbone::Device for quick access
      eb_address_t               my_msi_path;   // my msi path (required to configure mailbox)
      std::unique_ptr<Mailbox::Slot> my_slot;   // mailbox slot subscribed by me

      int                            bg_slot;   // mailbox slot-index subscribed by the burst generator
                                                // The slot is owned by the firmware and not by the host-side driver.
                                                // So no Mailbox::Slot object is used (it would release the slot on destruction)
                                                // this doesn't seem to be used 

      uint32_t                   response;      // instruction result and instruction code (sent by MSI)

      bool                       found_bg_fw;
      eb_address_t               ram_base;      // start of lm32 user ram
      std::vector<eb_address_t>  shm_buffer;    // app specific buffers in shared memory (for embedded lm32 communication)

  };

}

#endif
