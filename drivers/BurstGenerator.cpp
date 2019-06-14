#define ETHERBONE_THROWS 1

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
    found_bg_fw(false)
  {
    mb_slot = 0;
    ram_base = 0;

    std::vector<sdb_device> rom, ram;
    device.sdb_find_by_identity(LM32_RAM_USER_VENDOR, LM32_RAM_USER_PRODUCT, ram);
    device.sdb_find_by_identity(LM32_CLUSTER_ROM_VENDOR, LM32_CLUSTER_ROM_PRODUCT, rom);

    if (rom.size() != 1)
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "eCPU cluster ROM is missing");

    eb_address_t comp_addr = rom[0].sdb_component.addr_first;
    eb_data_t cpus;

    device.read(comp_addr, EB_DATA32, &cpus);

    if (cpus != ram.size())
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Mismatch between the number of eCPU RAMs and ROM cpu_count");

    found_bg_fw = detectFirmware(ram, BG_FW_ID, ram_base);

    if (found_bg_fw)
    {
      eb_data_t mb_slot_num;
      device.read(ram_base + SHM_MB_SLOT, EB_DATA32, &mb_slot_num);
      if (mb_slot_num < 0 || mb_slot_num >= MB_SLOT_RANGE)
        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "mailbox slot number is not in valid range");
      mb_slot = mailbox.sdb_component.addr_first + mb_slot * 8;
    }
    else
    {
      ram_base = 0;
      throw saftbus::Error(saftbus::Error::FAILED, "No valid firmware for BurstGenerator found");
    }
  }

  BurstGenerator::~BurstGenerator()
  {
  }

  std::shared_ptr<BurstGenerator> BurstGenerator::create(const ConstructorType& args)
  {
    return RegisteredObject<BurstGenerator>::create(args.objectPath, args);
  }

  bool BurstGenerator::detectFirmware(std::vector<sdb_device> ram, uint32_t id, eb_address_t &ram_base)
  {
    eb_data_t val;

    for (unsigned i = 0; i < ram.size(); ++i)
    {
      ram_base = ram[i].sdb_component.addr_first;

      device.read(ram_base + SHM_FW_ID, EB_DATA32, &val);

      if (id == static_cast<uint32_t>(val))
        return true;
    }

    return false;
  }

  uint32_t BurstGenerator::readFirmwareId()
  {
    eb_data_t bg_fw_id = 0;

    if (ram_base)
      device.read(ram_base + SHM_FW_ID, EB_DATA32, &bg_fw_id);

    clog << kLogDebug << "method call BurstGenerator::readFirmwareId succeeded" << std::endl;
    return static_cast<uint32_t>(bg_fw_id);
  }

  int32_t BurstGenerator::sendPulseParams(uint32_t eventHi, uint32_t delay, uint32_t conditions, uint32_t offset)
  {
    int32_t failed = -20;

    if (ram_base == 0)
      return failed;

    if (mb_slot == 0)
      return failed;

    try
    {
      // write argument data into the shared memory
      Cycle cycle;
      cycle.open(device);

      cycle.write(ram_base + SHM_INPUT, EB_DATA32, eventHi);
      cycle.write(ram_base + SHM_INPUT + 0x4, EB_DATA32, delay);
      cycle.write(ram_base + SHM_INPUT + 0x8, EB_DATA32, conditions);
      cycle.write(ram_base + SHM_INPUT + 0xc, EB_DATA32, offset);
      cycle.close();

      // instruct LM32 to obtain the argument data from the shared memory
      device.write(mb_slot, EB_DATA32, CMD_GET_PARAM); // load pulse parameters

      clog << kLogDebug << "method call BurstGenerator::sendPulseParams succeeded" << std::endl;
      return EB_OK;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "method call " << e.method << "failed with status " << e.status << std::endl;
      return e.status;
    }
  }

  int32_t BurstGenerator::sendProdCycles(uint32_t eventHi, uint64_t cycles)
  {
    int32_t failed = -20;

    if (ram_base == 0)
      return failed;

    if (mb_slot == 0)
      return failed;

    try
    {
      // write argument data into the shared memory
      Cycle cycle;
      cycle.open(device);

      cycle.write(ram_base + SHM_INPUT, EB_DATA32, eventHi);
      cycle.write(ram_base + SHM_INPUT + 0x4, EB_DATA32, cycles >> 32);
      cycle.write(ram_base + SHM_INPUT + 0x8, EB_DATA32, cycles & 0xFFFFFFFFUL);
      cycle.close();

      // instruct LM32 to obtain the argument data from the shared memory
      device.write(mb_slot, EB_DATA32, CMD_GET_CYCLE); // load production cycles

      clog << kLogDebug << "method call BurstGenerator::sendProdCycle succeeded" << std::endl;
      return EB_OK;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "method call " << e.method << "failed with status " << e.status << std::endl;
      return e.status;
    }
  }

  int32_t BurstGenerator::instruct(uint32_t code, const std::vector< uint32_t >& args)
  {
    int32_t failed = -20;

    if (ram_base == 0)
      return failed;

    if (mb_slot == 0)
      return failed;

    try
    {
      // write the instruction arguments into the shared memory
      Cycle cycle;
      cycle.open(device);

      cycle.write(ram_base + SHM_CMD, EB_DATA32, 0); // clear cmd register

      for (uint32_t i = 0; i < args.size(); ++i)
        cycle.write(ram_base + SHM_INPUT + (i << 2), EB_DATA32, args.at(i));

      cycle.close();

      // send the instruction code to LM32
      device.write(mb_slot, EB_DATA32, code);

      clog << kLogDebug << "method call BurstGenerator::instruct succeeded: " << code << std::endl;
      return EB_OK;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "method call" << e.method << "failed with status" << e.status << std::endl;
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
      eb_data_t data;
      device.read(ram_base + SHM_CMD, EB_DATA32, &data);
      if (static_cast<uint32_t>(data) != CMD_LS_BURST)
      {
        clog << kLogDebug << "More waits might be needed until LM32 uploads the burst info: " << static_cast<uint32_t>(data) << ' ' << CMD_LS_BURST << std::endl;
        return info;
      }

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

      clog << kLogDebug << "method call BurstGenerator::readBurstInfo succeeded: " << info.size() << std::endl;
      return info;
    }
    catch (etherbone::exception_t e)
    {
      clog << kLogDebug << "method call" << e.method << "failed with status" << e.status << std::endl;
      return info;
    }
  }
}

