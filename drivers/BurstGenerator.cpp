#define ETHERBONE_THROWS 1

#include <unistd.h>

#include "RegisteredObject.h"
/* create and register object_path to SAFTd
 * include class definitions of Device, etherbone
 */
#include "Output.h"
#include "BurstGenerator.h"
#include "TimingReceiver.h"
#include "clog.h"
#include "bg_regs.h"

using namespace etherbone;

namespace saftlib {

  BurstGenerator::BurstGenerator(const ConstructorType& args) :
    Owned(args.objectPath),
    tr(args.tr),
    device(args.device),
    sdb_msi_base(args.sdb_msi_base),
    mailbox(args.mailbox),
    my_msi_path(args.device.request_irq(args.sdb_msi_base, sigc::mem_fun(*this, &BurstGenerator::msi_handler))),
    response(0),
    found_bg_fw(false)
  {
    // detect lm32 cores
    std::vector<sdb_device> rom;
    device.sdb_find_by_identity(LM32_CLUSTER_ROM_VENDOR, LM32_CLUSTER_ROM_PRODUCT, rom);

    if (rom.size() != 1)
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "BurstGenerator: lm32 cluster ROM is missing.");

    std::vector<sdb_device> ram;
    device.sdb_find_by_identity(LM32_RAM_USER_VENDOR, LM32_RAM_USER_PRODUCT, ram);

    eb_address_t comp_addr = rom[0].sdb_component.addr_first;
    eb_data_t cpus;

    device.read(comp_addr, EB_DATA32, &cpus);

    if (cpus != ram.size())
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "BurstGenerator: mismatch between the number of eCPU RAMs and ROM cpu_count.");

    // detect a reset controller
    std::vector<struct sdb_device> reset_controllers;
    device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x3a362063), reset_controllers);

    if (reset_controllers.size() == 0)
      throw saftbus::Error(saftbus::Error::FAILED, "BurstGenerator: could not find a reset controller.");

    // find a free mailbox slot by checking its trigger registers
    my_slot = -1;
    int slot = 1;
    eb_data_t data;
    eb_address_t mb_base = mailbox.sdb_component.addr_first;

    while (slot < MB_SLOT_RANGE && data != my_msi_path && data != MB_SLOT_CFG_FREE)
    {
      device.read(mb_base + slot * 8, EB_DATA32, &data);
      slot++;
    }

    if (slot >= MB_SLOT_RANGE)
      throw saftbus::Error(saftbus::Error::FAILED, "BurstGenerator: no slot is available in mailbox.");

    // subscribe the mailbox slot (set own msi path to the configuration register)
    my_slot = --slot;
    device.write(mb_base + my_slot * 8 + 4, EB_DATA32, my_msi_path);
    clog << kLogDebug << "BurstGenerator: subscribed mailbox" << std::hex <<
      " slot: " << my_slot << " addr: " << mb_base + my_slot * 8 <<
      " path: " << my_msi_path << std::endl;

    clog << kLogDebug << "BurstGenerator: detecting the burst generator" << std::endl;

    // detect the burst generator
    int cpu_idx = -1;
    eb_data_t bg_id = 0;

    for (unsigned i = 0; i < ram.size(); ++i)
    {
      ram_base = ram[i].sdb_component.addr_first;

      // write own slot number to a reserved location (shared memory)
      device.write(ram_base + SHM_MB_SLOT_HOST, EB_DATA32, (eb_data_t)my_slot);

      // reset lm32 core
      uint32_t all_cpus = (1 << ram.size()) - 1;
      uint32_t bits = (1 << i) & all_cpus;

      clog << kLogDebug << "BurstGenerator: resetting lm32 core." <<
        " (idx = " << i << ", bits = " << bits <<
        ", all_cpus = " << all_cpus << ")" << std::endl;

      // clear a buffer for the FW ID (in the shared memory) before reset
      device.read(ram_base + SHM_FW_ID, EB_DATA32, &data);     // back up buffer value
      device.write(ram_base + SHM_FW_ID, EB_DATA32, 0);

      // use the first reset controller (register offsets: SET = +0x8, CLR = +0xc)
      device.write(reset_controllers[0].sdb_component.addr_first + 0x8, EB_DATA32, bits); // halt
      usleep(10);
      device.write(reset_controllers[0].sdb_component.addr_first + 0xc, EB_DATA32, bits); // resume

      // read the buffer for the FW ID until wait time expires (2 seconds)
      int timeout = 20;
      while ((static_cast<uint32_t>(bg_id) != BG_FW_ID) || (timeout != 0)) {
        usleep(100000);
        --timeout;
        device.read(ram_base + SHM_FW_ID, EB_DATA32, &bg_id);
      }

      clog << kLogInfo << "BurstGenerator: LM32 reset complete." << std::endl;

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

      bg_slot = mailbox.sdb_component.addr_first + data * 8;

      cpu_idx = i;

      clog << kLogInfo << "BurstGenerator: LM32 ram base = 0x" << std::hex << ram_base <<
        ", my mailbox slot = " << my_slot <<
        " is stored at 0x" << std::hex << (uint32_t)(ram_base + SHM_MB_SLOT_HOST) << " in the shared memory for LM32."  << std::endl;

      clog << kLogInfo << "BurstGenerator: firmware is running." << std::endl;
      break;
    }

    if (cpu_idx < 0)
    {
      ram_base = 0;
      bg_slot = 0;
      device.write(mb_base + my_slot * 8 + 4, EB_DATA32, MB_SLOT_CFG_FREE); // unsubscribe the mailbox slot
      throw saftbus::Error(saftbus::Error::FAILED, "BurstGenerator: could not detect the burst generator.");
    }

    clog << kLogDebug << "BurstGenerator: the burst generator id: " << bg_id << std::endl;
    clog << kLogDebug << "BurstGenerator: bg mailbox addr = " << bg_slot << ", lm32 cpu index = " << cpu_idx <<
      ", ram base = 0x" << std::hex << ram_base << std::endl;

  }

  BurstGenerator::~BurstGenerator()
  {
    if (0 < my_slot && my_slot < MB_SLOT_RANGE)
    {
      // unsubscribe the mailbox slot
      eb_address_t my_mb_config = mailbox.sdb_component.addr_first + my_slot * 8 + 4;
      device.write(my_mb_config, EB_DATA32, MB_SLOT_CFG_FREE);
    }

    // write the invalid slot number to a reserved location in the shared memory
    device.write(ram_base + SHM_MB_SLOT_HOST, EB_DATA32, MB_SLOT_CFG_FREE);

    // release MSI handler
    device.release_irq(my_msi_path);
  }

  std::shared_ptr<BurstGenerator> BurstGenerator::create(const ConstructorType& args)
  {
    return RegisteredObject<BurstGenerator>::create(args.objectPath, args);
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

      clog << kLogDebug << "BurstGenerator: method call instruct(" << code << ") succeeded." << std::endl;
      return EB_OK;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
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
        clog << kLogDebug << ' ' << static_cast<uint32_t>(data) << std::endl;
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

      clog << kLogDebug << "BurstGenerator: method call readBurstInfo(" << info.size() << ") succeeded."<< std::endl;
      return info;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
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

      clog << kLogDebug << "BurstGenerator: method call readSharedBuffer(" << content.size() << ") succeeded." << std::endl;

      return content;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
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

      clog << kLogDebug << "BurstGenerator: method call readState() succeeded: " << static_cast<uint32_t>(data) << std::endl;

      return static_cast<uint32_t>(data);
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "BurstGenerator: method call " << e.method << " failed with status: " << e.status << std::endl;
      return -1;
    }
  }

  uint32_t BurstGenerator::getResponse() const
  {
    return response;
  }

  void BurstGenerator::msi_handler(eb_data_t msg)
  {
    //clog << kLogDebug << "BurstGenerator: msi_handler(" << msg << ") called." << std::endl;
    //if (!getOwner().empty())
    {
      response = (uint32_t)msg;
      sigInstComplete(response);
      clog << kLogInfo << "BurstGenerator: signal sigInstComplete(" << response << ") emitted." << std::endl;
    }
  }

}

