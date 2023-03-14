#define ETHERBONE_THROWS 1

#include <unistd.h>

//#include "RegisteredObject.h"
#include "BurstGenerator.hpp"
#include "bg_regs.h"
//#include "clog.h"

// #include <Output.hpp>
#include <TimingReceiver.hpp>
#include <SAFTd.hpp>

using namespace etherbone;

namespace saftlib {

  BurstGenerator::BurstGenerator(saftbus::Container *container, SAFTd *saft_daemon, TimingReceiver *timing_receiver)
    : Owned(container)
    , objectPath(timing_receiver->getObjectPath()+"/bg_firmware")
    , saftd(saft_daemon)
    , tr(timing_receiver)
    , device(tr->OpenDevice::get_device())
  {
    Mailbox *mbox = static_cast<Mailbox*>(tr); 
    my_msi_path = saftd->request_irq(*mbox, std::bind(&BurstGenerator::msi_handler,this, std::placeholders::_1));
    // For some reason it doesn't work if slot index is 0 (maybe slot index 0 has a special meaning in the firmware).
    // Fix it simply by reconfiguring the mailbox if the assigned slot index was 0.
    do { 
      my_slot = mbox->ConfigureSlot(my_msi_path);
    } while (my_slot->getIndex() == 0);

    std::cerr << "BurstGenerator: subscribed mailbox" << std::hex <<
      " slot: " << my_slot->getIndex() << " addr: " << my_slot->getAddress() <<
      " path: " << my_msi_path << std::endl;

    std::cerr << "BurstGenerator: detecting the burst generator" << std::endl;

    eb_data_t bg_id = 0;
    int cpu_idx = -1;

    for (unsigned i = 0; i < tr->LM32Cluster::getCpuCount(); ++i)
    {
      ram_base = timing_receiver->LM32Cluster::dpram_lm32_adr_first[i];

      // write own slot number to a reserved location (shared memory)
      device.write(ram_base + SHM_MB_SLOT_HOST, EB_DATA32, (eb_data_t)my_slot->getIndex());


      std::cerr << "BurstGenerator: resetting lm32 core." << i << std::endl;

      // clear a buffer for the FW ID (in the shared memory) before reset
      eb_data_t data;
      device.read(ram_base + SHM_FW_ID, EB_DATA32, &data);     // back up buffer value
      device.write(ram_base + SHM_FW_ID, EB_DATA32, 0);

      // use the first reset controller (register offsets: SET = +0x8, CLR = +0xc)
      tr->Reset::CpuHalt(i);
      usleep(10);
      tr->Reset::CpuReset(i);

      // read the buffer for the FW ID until wait time expires (2 seconds)
      int timeout = 20;
      while ((static_cast<uint32_t>(bg_id) != BG_FW_ID) && (timeout != 0)) {
        usleep(100000);
        --timeout;
        device.read(ram_base + SHM_FW_ID, EB_DATA32, &bg_id);
      }

      std::cerr << "BurstGenerator: LM32 reset complete." << std::endl;

      if (static_cast<uint32_t>(bg_id) != BG_FW_ID)
      {
        device.write(ram_base + SHM_FW_ID, EB_DATA32, data);   // restore the buffer value
        continue;
      }

      // get a mailbox slot subscribed by the burst generator
      device.read(ram_base + SHM_MB_SLOT, EB_DATA32, &data);
      if (data < 0 || data >= MB_SLOT_RANGE)
      {
        continue;
      }
      bg_slot = data;
      cpu_idx = i;

      std::cerr << "BurstGenerator: LM32 ram base = 0x" << std::hex << ram_base <<
        ", my mailbox slot = " << my_slot->getIndex() <<
        " is stored at 0x" << std::hex << (uint32_t)(ram_base + SHM_MB_SLOT_HOST) << " in the shared memory for LM32."  << std::endl;

      std::cerr << "BurstGenerator: firmware is running." << std::endl;
      break;
    }

    std::cerr << "BurstGenerator: the burst generator id: " << bg_id << std::endl;
    std::cerr << "BurstGenerator: bg mailbox addr = " << bg_slot << ", lm32 cpu index = " << cpu_idx <<
      ", ram base = 0x" << std::hex << ram_base << std::endl;

  }

  BurstGenerator::~BurstGenerator()
  {
    // write the invalid slot number to a reserved location in the shared memory
    device.write(ram_base + SHM_MB_SLOT_HOST, EB_DATA32, MB_SLOT_CFG_FREE);

    // release MSI handler
    saftd->release_irq(my_msi_path);
  }


  std::string BurstGenerator::getObjectPath() 
  {
    return objectPath;
  }

  std::map<std::string, std::map<std::string, std::string> > BurstGenerator::getObjects() {
    std::map<std::string, std::map<std::string, std::string> > result;
    result["BurstGenerator"]["bg_firmware"] = objectPath;
    return result;
  }

  bool BurstGenerator::firmwareRunning(uint32_t id)
  {
    if (ram_base == 0)
      return false;

    if (bg_slot == 0)
      return false;

    // send an user command to get the firmware ID (wait 100 ms for the firmware response)
    int timeout = 10;
    eb_data_t data = 0;

    device.write(ram_base + SHM_INPUT, EB_DATA32, data);
    device.write(ram_base + SHM_CMD, EB_DATA32, CMD_LS_FW_ID);

    while (( data == 0) || (timeout != 0)) {
      usleep(10000);
      --timeout;
      device.read(ram_base + SHM_INPUT, EB_DATA32, &data);
    }

    return (id == (uint32_t)data);
  }

  int32_t BurstGenerator::instruct(uint32_t code, const std::vector< uint32_t >& args)
  {
    int32_t failed = -20;

    if (ram_base == 0)
      return failed;

    if (bg_slot == 0)
      return failed;

    try
    {
      // write the instruction arguments into the shared memory
      etherbone::Cycle cycle;
      cycle.open(device);

      cycle.write(ram_base + SHM_CMD, EB_DATA32, 0); // clear cmd register

      for (uint32_t i = 0; i < args.size(); ++i)
        cycle.write(ram_base + SHM_INPUT + (i << 2), EB_DATA32, args.at(i));

      cycle.close();

      // send the instruction code to LM32
      device.write(ram_base + SHM_CMD, EB_DATA32, code);

      std::cerr << "BurstGenerator: method call instruct(0x" << std::hex << code << std::dec << ") succeeded." << std::endl;
      return EB_OK;
    }
    catch (etherbone::exception_t e)
    {
      std::cerr << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
      return e.status;
    }
  }

  std::vector< uint32_t > BurstGenerator::readBurstInfo(uint32_t id)
  {
    std::vector<uint32_t> info;

    if (ram_base == 0)
      return info;

    try
    {
      // common-libs deletes the command buffer in the shared memory
      eb_data_t data;

      // read burst info from the shared memory
      /*TODO: cycle read failed! Find out the reason!
      Cycle cycle;
      cycle.open(device);

      for (int i = 0; i < N_BURST_INFO; ++i)
      {
        cycle.read(ram_base + SHM_INPUT + (i << 2), EB_DATA32, &data);
        std::cerr << ' ' << static_cast<uint32_t>(data) << std::endl;
        info.push_back(static_cast<uint32_t>(data));
      }

      cycle.close();

      info.clear();*/

      if (id == 0)
      {
        device.read(ram_base + SHM_INPUT, EB_DATA32, &data); // created bursts
        info.push_back(static_cast<uint32_t>(data));
        device.read(ram_base + SHM_INPUT + 4, EB_DATA32, &data); // cycled bursts
        info.push_back(static_cast<uint32_t>(data));
      }
      else if (id <= N_BURSTS)
      {
        for (int i = 0; i < N_BURST_INFO; ++i)
        {
          device.read(ram_base + SHM_INPUT + (i << 2), EB_DATA32, &data);
          info.push_back(static_cast<uint32_t>(data));
        }
      }

      std::cerr << "BurstGenerator: method call readBurstInfo(" << info.size() << ") succeeded."<< std::endl;
      return info;
    }
    catch (etherbone::exception_t e)
    {
      std::cerr << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
      return info;
    }
  }

  std::vector< uint32_t > BurstGenerator::readSharedBuffer(uint32_t size)
  {
    std::vector<uint32_t> content;

    if (ram_base == 0)
      return std::vector<uint32_t>();

    try
    {
      eb_data_t data;

      // read the shared memory
      for (unsigned int i = 0; i < size; ++i)
      {
        device.read(ram_base + SHM_INPUT + (i << 2), EB_DATA32, &data);
        content.push_back(static_cast<uint32_t>(data));
      }

      std::cerr << "BurstGenerator: method call readSharedBuffer(" << content.size() << ") succeeded." << std::endl;

      return content;
    }
    catch (etherbone::exception_t e)
    {
      std::cerr << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
      return content;
    }
  }

  uint32_t BurstGenerator::readState()
  {
    if (ram_base == 0)
      return COMMON_STATE_UNKNOWN;

    try
    {
      eb_data_t data;

      // read the FSM state location in the shared memory
      device.read(ram_base + SHM_STATE, EB_DATA32, &data);

      std::cerr << "BurstGenerator: method call readState() succeeded: " << static_cast<uint32_t>(data) << std::endl;

      return static_cast<uint32_t>(data);
    }
    catch (etherbone::exception_t e)
    {
      std::cerr << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
      return -1;
    }
  }

  uint32_t BurstGenerator::getResponse() const
  {
    return response;
  }

  void BurstGenerator::msi_handler(eb_data_t msg)
  {
    //std::cerr << "BurstGenerator: msi_handler(" << msg << ") called." << std::endl;
    //if (!getOwner().empty())
    {
      response = (uint32_t)msg;
      sigInstComplete(response);
      std::cerr << "BurstGenerator: signal sigInstComplete(" << response << ") emitted." << std::endl;
    }
  }

}

