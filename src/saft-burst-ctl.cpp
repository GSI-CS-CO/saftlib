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
/* Synopsis */
/* ==================================================================================================== */
/* Burst control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#define ECA_EVENT_ID_LATCH     UINT64_C(0xfffe000000000000) /* FID=MAX & GRPID=MAX-1 */
#define ECA_EVENT_MASK_LATCH   UINT64_C(0xfffe000000000000)
#define IO_CONDITION_OFFSET    5000

#define BG_MIN_PERIOD          1000UL                       /* period of a signal, nanoseconds */
#define BG_MIN_BLOCK_PERIOD    100000UL                     /* period of a pulse block that can be generated repeatedly by LM32, nanoseconds */
#define BG_MAX_RULES_PER_IO    100                          /* max number of conditions for a burst type */

/* Includes */
/* ==================================================================================================== */
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <string>
#include <unistd.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/Output.h"
#include "interfaces/Input.h"
#include "interfaces/OutputCondition.h"
#include "interfaces/EmbeddedCPUActionSink.h"
#include "interfaces/EmbeddedCPUCondition.h"
#include "interfaces/BurstGenerator.h"

#include "src/CommonFunctions.h"

#include "drivers/eca_flags.h"
#include "drivers/io_control_regs.h"
#include "drivers/bg_regs.h"

/* Namespace */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Global */
/* ==================================================================================================== */
static const char   *program      = NULL;  /* Name of the application */
static const char   *deviceName   = NULL;  /* Name of the device */
static const char   *ioName       = NULL;  /* Name of the IO */
static bool          ioNameGiven  = false; /* IO name given? */
static bool          ioNameExists = false; /* IO name does exist? */
std::map<std::string,uint64_t>     map_PrefixName; /* Translation table IO name <> prefix */
static int           burstId      = 0;     /* burst ID */
static bool          burstIdGiven = false; /* is burst ID given? */

/* Prototypes */
/* ==================================================================================================== */
static void io_help        (void);
static int  io_setup       (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_gate_out, int io_gate_in, int io_mux, int io_pps, int io_drive, int stime,
                            bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_gate_out, bool set_gate_in, bool set_mux, bool set_pps, bool set_drive, bool set_stime,
                            bool verbose_mode);
static int  io_create      (bool disown, uint64_t eventID, uint64_t eventMask, int64_t offset, uint64_t flags, int64_t level, bool offset_negative, bool translate_mask);
static int  io_destroy     (bool verbose_mode);
static int  io_flip        (bool verbose_mode);
static int  io_list        (void);
static int  io_list_i_to_e (void);
static int  io_print_table (bool verbose_mode);
static void io_catch_input (uint64_t event, uint64_t param, uint64_t deadline, uint64_t executed, uint16_t flags);
static int  io_snoop       (bool mode, bool setup_only, bool disable_source, uint64_t prefix_custom);

static int  bg_read_fw_id       (void);
static int  bg_instruct         (std::vector<std::string> instr);
static int  bg_config_io        (uint32_t t_high, uint32_t t_period, int64_t t_burst, uint64_t b_delay, uint32_t b_flag, bool verbose_mode);
static int  bg_get_io_name      (int burst_id, std::string &name);
static int  bg_list_bursts      (int burst_id, bool verbose);
static int  bg_remove_burst     (int burst_id, bool verbose);
static int  bg_disenable_burst  (int burst_id, int disen, bool verbose);
static int  bg_create_burst     (int burst_id, uint64_t e_id, uint64_t e_mask, bool verbose);
static std::string find_io_name (std::shared_ptr<TimingReceiver_Proxy> tr, std::string type, std::vector<uint32_t> info);
static int  ecpu_update         (uint64_t e_id, uint64_t e_mask, int64_t offset, uint32_t tag, uint32_t toggle, bool verbose);
static int  ecpu_check          (uint64_t e_id, uint64_t e_mask, int64_t offset, uint32_t tag, uint32_t toggle);
static int  ecpu_destroy        (bool verbose_mode);
static int  bg_clear_all        (bool verbose);

/* Get firmware id of the burst generator */
/* ==================================================================================================== */
static int bg_read_fw_id(void)
{
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware)
      std::cout << "Firmware ID: " << std::hex << bg_firmware->readFirmwareId() << std::endl;
    else
      std::cerr << "Failed to get firmware ID of burst generator" << std::endl;
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return 0;
}

/* Instruct the burst generator */
/* ==================================================================================================== */
static int bg_instruct(std::vector<std::string> instr)
{
  if (instr.empty())
  {
    std::cerr << "Missing arguments for method call bg_instruct()" << std::endl;
    return -1;
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware)
    {
      std::vector<std::string>::iterator it = instr.begin();

      std::vector<uint32_t> instr_args;
      uint32_t instr_code = strtoul((*it).c_str(), NULL, 0);
      ++it;

      while (it != instr.end())
      {
        instr_args.push_back(strtoul((*it).c_str(), NULL, 0));
        ++it;
      }

      int res = bg_firmware->instruct(instr_code, instr_args);
      if (res)
        std::cerr << "Failed method call bg_instruct()!" << std::endl;
    }
    else
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return 0;
}

/* Find the IO port name based on the given burst info and port type */
static std::string find_io_name(std::shared_ptr<TimingReceiver_Proxy> tr, std::string type, std::vector<uint32_t> info)
{
  std::map<std::string, std::string> outs = tr->getOutputs();
  std::shared_ptr<Output_Proxy> out_proxy;

  for (std::map<std::string,std::string>::iterator it = outs.begin(); it != outs.end(); ++it)
  {
    out_proxy = Output_Proxy::create(it->second);

    if (out_proxy != NULL)
    {
      std::size_t found = out_proxy->getTypeOut().find(type);
      if (found != std::string::npos)
      {
        if (out_proxy->getIndexOut() == info.at(2))
          return it->first;
      }
    }
  }

  return std::string();
}
/* Determine the IO port name from the burst declaration */
static int bg_get_io_name(int burst_id, std::string &name)
{
  if (burst_id == 0)
  {
    std::cerr << "Invalid burst ID!" << std::endl;
    return -1;
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    /* Create a proxy for the burst generator */
    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Failed to create a proxy for the burst generator driver" << std::endl;
      return -1;
    }

    std::vector<uint32_t> info;
    info.push_back(burst_id);

    int res = bg_firmware->instruct(CMD_LS_BURST, info);
    if (res)
      std::cerr << "Failed method call bg_instruct()!" << std::endl;

    sleep(1); // let LM32 complete the previous command

    info = bg_firmware->readBurstInfo(burst_id);
    if (info.size() == 0)
    {
      std::cerr << "Failed to get burst info" << std::endl;
      return -1;
    }

    name = find_io_name(tr, "8ns", info); // TODO: replace literals with a constant!
    return 0;
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return 0;
}

/* List currently enabled bursts */
static int bg_list_bursts(int burst_id, bool verbose_mode)
{
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Could not create a proxy for the burst generator driver" << std::endl;
      return -1;
    }

    std::vector<uint32_t> args;
    args.push_back(burst_id);
    args.push_back((uint32_t)verbose_mode);

    int res = bg_firmware->instruct(CMD_LS_BURST, args);
    if (res)
    {
      std::cerr << "Failed to list currently enabled bursts" << std::endl;
      return -1;
    }

    if (verbose_mode) // TODO: replace sleep with a better method if possible!
      sleep(2);
    else
      sleep(1);

    args = bg_firmware->readBurstInfo(burst_id);

    if (args.empty())
    {
      std::cerr << "Something went wrong on reading the burst info! Try again." << std::endl;
      return -1;
    }

    if (burst_id)
    {
      std::cout << "Burst info (hex):";
      for (unsigned int i = 0; i < args.size(); ++i)
        std::cout << ' ' << std::hex << args.at(i);
      std::cout << std::endl;
      if (verbose_mode && args.size() == N_BURST_INFO)
      {
        std::cout << "Burst info (text):";
        std::string name = find_io_name(tr, "8ns", args);
        std::cout << " id = " << args.at(0) << ", IO = " << name <<
          ", trig = " << std::hex << args.at(3) << ':' << args.at(4) <<
          ", togg = " << args.at(5) << ':' << args.at(6) <<
          ", enabled = " << ((args.at(7) & CTL_EN) ? "yes" : "no") << std::endl;
      }
    }
    else
    {
      std::cout << "Created " << args.at(0) << " : ";
      uint32_t mask = 1;
      int id = 1;

      while (mask != 0)
      {
        if (args.at(0) & mask)
          std::cout << ' ' << id;
        ++id;
        mask <<=1;
      }
      std::cout << std::endl;

      std::cout << "Cycled " << args.at(1) << " : ";
      mask = 1; id = 1;
      while (mask != 0)
      {
        if (args.at(1) & mask)
          std::cout << ' ' << id;
        ++id;
        mask <<=1;
      }
      std::cout << std::endl;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return 0;
}

/* Remove burst */
static int bg_remove_burst(int burst_id, bool verbose)
{
  int result = 0;

  // destroy all conditions for the chosen burst (IO)
  std::string name;

  if (bg_get_io_name(burst_id, name))
  {
    std::cout << "Failed to determine IO name" << std::endl;
    return -1;
  }

  if (name.empty())
  {
    std::cout << "Could not determine IO name" << std::endl;
    return -1;
  }

  ioName = name.c_str();
  ioNameGiven = true;

  result = io_destroy(true);

  if (result != 0)
    std::cerr << "Failed to destroy conditions!" << std::endl;

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
      return -1;
    }

    std::vector<uint32_t> args;
    args.push_back(burst_id);
    args.push_back(static_cast<uint32_t>(verbose));

    int res = bg_firmware->instruct(CMD_RM_BURST, args);
    if (res)
    {
      std::cerr << "Failed to disable burst" << std::endl;
      return -1;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}


/* Dis/enable burst */
static int bg_disenable_burst(int burst_id, int disen, bool verbose)
{
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
      return -1;
    }

    std::vector<uint32_t> args;
    args.push_back(burst_id);
    args.push_back(disen);
    args.push_back(static_cast<uint32_t>(verbose));

    int res = bg_firmware->instruct(CMD_DE_BURST, args);
    if (res)
    {
      std::cerr << "Failed to disable burst" << std::endl;
      return -1;
    }
 }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return 0;
}

/* Create new burst */
static int bg_create_burst(int burst_id, uint64_t e_id, uint64_t e_mask, bool verbose)
{
  int result = 0;

  if (ioNameGiven == false || ioName == NULL)
  {
    std::cerr << "Missing IO name!" << std::endl;
    return -1;
  }

  if (!burstIdGiven)
  {
    std::cerr << "Missing burst ID!" << std::endl;
    return -1;
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    /* Create a proxy for the burst generator */
    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Failed to create a proxy for the burst generator driver" << std::endl;
      return -1;
    }

    std::map<std::string, std::string> outs = tr->getOutputs();
    std::shared_ptr<Output_Proxy> out_proxy = NULL;

    std::map<std::string,std::string>::iterator it = outs.begin();
    while (out_proxy == NULL && it != outs.end())
    {
      if (it->first == ioName)
        out_proxy = Output_Proxy::create(it->second);
      ++it;
    }

    if (out_proxy == NULL)
    {
      std::cerr << "Not found the IO port " << ioName << std::endl;
      return -1;
    }

    /* Check if the same conditions for eCPU actions exist already */
    int check = ecpu_check(e_id, e_mask, 0, BG_FW_ID, 1);

    if (check < 0)
    {
      std::cerr << "Failed to check conditions for eCPU actions" << std::endl;
      return -1;
    }
    else if (check == 0) // conditions were not set
    {
      /* Configure ECA with the conditions for eCPU actions */
      if (ecpu_update(e_id, e_mask, 0, BG_FW_ID, 1, verbose))  // TODO: apply offset? evaluate toggle flag
      {
        std::cerr << "Failed to set conditions for eCPU actions" << std::endl;
        return -1;
      }
    }

    /* Set up IO port */
    int io_oe = 1;
    int io_drive = 0;
    bool set_oe = true;
    bool set_drive = true;

    result = io_setup(io_oe, 0, 0, 0, 0, 0, 0, 0, io_drive, 0,
          set_oe, false, false, false, false, false, false, false, set_drive, false, true);

    if (result != 0)
    {
      std::cerr << "Failed to set up IO: " << ioName << std::endl;
      return result;
    }

    std::vector<uint32_t> args;
    args.push_back(burst_id);
    args.push_back(out_proxy->getIndexOut()); // TODO: add io type (missing a proper member function in InoutImpl)
    args.push_back((uint32_t)(e_id >> 32));
    args.push_back((uint32_t)e_id);
    uint64_t toggle_id = e_id ^ 0x0000001000000000; // TODO: eliminate fixed toggle event id
    args.push_back((uint32_t)(toggle_id >> 32));
    args.push_back((uint32_t)toggle_id);
    args.push_back(static_cast<uint32_t>(verbose));

    int res = bg_firmware->instruct(CMD_MK_BURST, args);
    if (res)
    {
      std::cerr << "Failed to enable burst" << std::endl;
      return -1;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}


/* Configure ECA with the IO event conditions for generating pulse at the chosen output */
/* ==================================================================================================== */
static int  bg_config_io(uint32_t t_high, uint32_t t_period, int64_t t_burst, uint64_t b_delay, uint32_t b_flag, bool verbose_mode)
{
  int result = 0;

  // check arguments
  if (ioNameGiven == false || ioName == NULL)
  {
    if (burstId == 0)
    {
      std::cerr << "Missing IO name!" << std::endl;
      return -1;
    }
  }

  if (!burstIdGiven)
  {
    std::cerr << "Missing burst ID!" << std::endl;
    return -1;
  }

  if (t_period == 0)
  {
    std::cerr << "Bad arguments: t_period = " << t_period << std::endl;
    return -1;
  }

  if (t_period < BG_MIN_PERIOD)
  {
    std::cerr << "Signal period is too short!" << t_period << std::endl;
    return -1;
  }

  uint32_t conditions = 2;
  uint32_t block_period = t_period;

  if (t_high == 0 || t_high == t_period)
  {
    conditions = 1;
  }
  else if (t_period < BG_MIN_BLOCK_PERIOD)
  {

    while(block_period < BG_MIN_BLOCK_PERIOD)
    {
      block_period += t_period;
      conditions += 2;
    }
  }

  if (conditions > BG_MAX_RULES_PER_IO)
  {
    std::cerr << "Number of conditions exceeds the allowed limit (" << BG_MAX_RULES_PER_IO << ") :" << conditions << std::endl;
    return -1;
  }

  int64_t cycles;

  if (t_burst > 0)
  {
    cycles = t_burst / block_period;
    if (t_burst % block_period)
      cycles++;
  }
  else
    cycles = -1; //std::numeric_limits<int64_t>::min();

  std::string name;

  if (bg_get_io_name(burstId, name))
  {
    std::cout << "Failed to determine IO name" << std::endl;
    return -1;
  }

  if (name.empty())
  {
    std::cout << "Could not determine IO name" << std::endl;
    return -1;
  }

  ioName = name.c_str();
  ioNameGiven = true;

  if (verbose_mode)
  {
    std::cout << "conds (dec) = " << conditions << ", block period = " << block_period << ", cycles = " << cycles << std::endl;
    std::cout << "conds (hex) = " << std::hex << conditions << ", block period = " << block_period << ", cycles = " << cycles << std::endl;
  }

  bool disown = true;
  uint64_t flags  = 0xf;
  uint64_t level  = 1;

  uint64_t offset = b_delay;
  uint64_t t_low = t_period - t_high;

  uint64_t _e_id = EVT_ID_IO_H32 + (burstId << 4);
  _e_id <<= 32;
  uint64_t _e_mask = EVT_MASK_IO;
  if (conditions == 1)
  {
    if (t_high == t_period)
      level = 1;
    else
      level = 0;

    result = io_create(disown, _e_id, _e_mask, offset, flags, level, false, false);

    if (result != 0)
    {
      std::cerr << "Failed to create a condition for IO action: id = " << std::hex << _e_id << "mask = " << _e_mask << std::endl;
      io_destroy(true); // destroy conditions that were created
      return result;
    }
    else if (verbose_mode)
      std::cout << "_e_id = " << std::hex << _e_id << "_e_msk = " << _e_mask << std::dec << " offset = " << offset << " level = " << level << std::endl;
  }
  else
  {
    for (uint32_t i = 1; i <= conditions; i+=2)
    {
      result = io_create(disown, _e_id, _e_mask, offset, flags, level, false, false);

      if (result != 0)
      {
        std::cerr << "Failed to create conditions for IO action: id = " << std::hex << _e_id << "mask = " << _e_mask  << std::endl;
        io_destroy(true); // destroy conditions that were created
        return result;
      }
      else if (verbose_mode)
        std::cout << "_e_id = " << std::hex << _e_id << " _e_msk = " << _e_mask << std::dec << " offset = " << offset << " level = " << level << std::endl;

      offset += t_high;

      level ^=1;

      result = io_create(disown, _e_id, _e_mask, offset, flags, level, false, false);

      if (result != 0)
      {
        std::cerr << "Failed to create conditions for IO action" << std::endl;
        io_destroy(true); // destroy conditions that were created
        return result;
      }
      else if (verbose_mode)
        std::cout << "_e_id = " << std::hex << _e_id << " _e_msk = " << _e_mask << std::dec <<" offset = " << offset << " level = " << level << std::endl;

      offset += t_low;

      level ^=1;
    }
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> bg_iface = tr->getInterfaces()["BurstGenerator"];
    if (bg_iface.empty())
    {
      std::cerr << "No BurstGenerator firmware found" << std::endl;
      return -1;
    }

    std::shared_ptr<BurstGenerator_Proxy> bg_firmware = BurstGenerator_Proxy::create(bg_iface.begin()->second);

    if (bg_firmware == NULL)
    {
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
      return -1;
    }

    std::vector<uint32_t> args;

    // pack burst parameters (id, delay, conditions, block period, burst flag)
    args.push_back(burstId);
    args.push_back(0);
    args.push_back(conditions);
    args.push_back(block_period);
    args.push_back(b_flag);
    args.push_back((uint32_t)verbose_mode);

    std::cout << "Pulse params: ";
    for ( std::vector<uint32_t>::iterator it = args.begin(); it != args.end(); ++it)
      std::cout << std::hex << *it << ' ';
    std::cout << std::endl;

    int res = bg_firmware->instruct(CMD_GET_PARAM, args);
    if (res)
    {
      std::cerr << "Failed to send pulse parameters" << std::endl;
      return -1;
    }

    sleep(1); // wait until LM32 reads from RAM, TODO: optimize it later!

    args.clear();

    // pack production cycles
    args.push_back(burstId);
    args.push_back(cycles >> 32);
    args.push_back(cycles);
    args.push_back((uint32_t)verbose_mode);

    std::cout << "Production cycles: ";
    for ( std::vector<uint32_t>::iterator it = args.begin(); it != args.end(); ++it)
      std::cout << std::hex << *it << ' ';
    std::cout << std::endl;

    res = bg_firmware->instruct(CMD_GET_CYCLE, args);
    if (res)
    {
      std::cerr << "Failed to send production cycles" << std::endl;
      return -1;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}

static int ecpu_destroy(bool verbose_mode)
{
  int result = 0;

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> ecpus = tr->getInterfaces()["EmbeddedCPUActionSink"];
    if (ecpus.empty())
    {
      std::cerr << "No embedded CPU found!" << std::endl;
      return -1;
    }

    /* Get connection */
    std::shared_ptr<EmbeddedCPUActionSink_Proxy> ecpu = EmbeddedCPUActionSink_Proxy::create(ecpus.begin()->second);

    /* Create the action sink */
    if (ecpu)
    {
      std::shared_ptr<EmbeddedCPUCondition_Proxy> condition;
      std::string ecpu_sink;
      std::vector<std::string> conditions = ecpu->getAllConditions();

      for (unsigned int it = 0; it < conditions.size(); it++)
      {
        ecpu_sink = conditions[it];

        condition = EmbeddedCPUCondition_Proxy::create(ecpu_sink);
        if (condition->getDestructible() && (condition->getOwner() == ""))
        {
          condition->Destroy();
          if (verbose_mode)
            std::cout << "Destroyed " << ecpu_sink << std::endl;
        }
        else
        {
          if (verbose_mode)
            std::cout << "Found not destructible " << ecpu_sink << std::endl;
        }
      }
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}

/* Configure ECA with the event conditions for eCPU actions */
static int  ecpu_update(uint64_t e_id, uint64_t e_mask, int64_t offset, uint32_t tag, uint32_t toggle, bool verbose)
{
  int result = 0;

  // request to destroy all conditions for the chosen IO
  if (e_id == 0)
  {
    result = ecpu_destroy(verbose);
    if (result != 0)
      std::cerr << "Failed to destroy conditions!" << std::endl;

    return result;
  }

  if (tag == 0)
  {
    std::cerr << "Bad arguments: tag = " << std::hex << tag << std::endl;
    return -1;
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> ecpus = tr->getInterfaces()["EmbeddedCPUActionSink"];
    if (ecpus.empty())
    {
      std::cerr << "No embedded CPU found!" << std::endl;
      return -1;
    }

    /* Get connection */
    std::shared_ptr<EmbeddedCPUActionSink_Proxy> ecpu = EmbeddedCPUActionSink_Proxy::create(ecpus.begin()->second);

    /* Create the action sink */
    if (ecpu)
    {
      std::vector<std::string> conditions;
      std::shared_ptr<EmbeddedCPUCondition_Proxy> condition;
      std::string ecpu_sink;

      uint64_t e_id_toggle = e_id;
      int n_conditions = 1;
      if (toggle)
        n_conditions++;

      for (int i = 0; i < n_conditions; ++i)
      {
        ecpu_sink = ecpu->NewCondition(true, e_id_toggle, e_mask, offset, tag);
        condition = EmbeddedCPUCondition_Proxy::create(ecpu_sink);

        condition->setAcceptConflict(true);
        condition->setAcceptDelayed(true);
        condition->setAcceptEarly(true);
        condition->setAcceptLate(false);  // TODO: find out why late action cannot be popped from the eCPU queue
        condition->Disown();
        e_id_toggle ^= 0x0000001000000000;
      }

    }
    else
    {
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
      return -1;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}

/* Check if the given conditions for eCPU actions is already set */
static int  ecpu_check(uint64_t e_id, uint64_t e_mask, int64_t offset, uint32_t tag, uint32_t toggle)
{
  int result = 0;

  // check arguments
  if (ioNameGiven == false || ioName == NULL)
  {
    std::cerr << "Missing IO name!" << std::endl;
    return -1;
  }

  // request to destroy all conditions for the chosen IO
  if (e_id == 0)
  {
    result = ecpu_destroy(true);
    if (result != 0)
      std::cerr << "Failed to destroy conditions!" << std::endl;

    return result;
  }

  if (tag == 0)
  {
    std::cerr << "Bad arguments: tag = " << std::hex << tag << std::endl;
    return -1;
  }

  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> tr = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Check if timing receiver has dedicated interfaces */
    map<std::string, std::string> ecpus = tr->getInterfaces()["EmbeddedCPUActionSink"];
    if (ecpus.empty())
    {
      std::cerr << "No embedded CPU found!" << std::endl;
      return -1;
    }

    /* Get connection */
    std::shared_ptr<EmbeddedCPUActionSink_Proxy> ecpu = EmbeddedCPUActionSink_Proxy::create(ecpus.begin()->second);

    /* Create the action sink */
    if (ecpu)
    {
      /* Check if desired conditions already exist */
      std::vector<std::string> conditions = ecpu->getAllConditions();

      for (unsigned int c = 0; c < conditions.size(); c++)
      {
        std::shared_ptr<EmbeddedCPUCondition_Proxy> c_proxy = EmbeddedCPUCondition_Proxy::create(conditions[c]);

        if (c_proxy)
        {
          if (c_proxy->getID() == e_id && c_proxy->getMask() == e_mask &&
              c_proxy->getTag() == tag && c_proxy->getOffset() == offset)
          {
            std::cerr << "Trigger condition for eCPU already exists!" << std::endl;
            return 1;
          }

          if (toggle) // TODO: eliminate hard coded toggle event id
          {
            uint64_t toggled_id = e_id ^ 0x0000001000000000;
            if (c_proxy->getID() == toggled_id && c_proxy->getMask() == e_mask &&
              c_proxy->getTag() == tag && c_proxy->getOffset() == offset)
            {
              std::cerr << "Toggle condition for eCPU already exists!" << std::endl;
              return 2;
            }
          }
        }
      }
    }
    else
    {
      std::cerr << "Could not connect to the burst generator driver" << std::endl;
      return -1;
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return -1;
  }

  return result;
}

/* Clear all unowned conditions for the IO and eCPU actions */
static int bg_clear_all(bool verbose)
{
  int result = ecpu_update(0, 0, 0, 0, 0, verbose);
  result |= io_destroy(verbose);
  return result;
}

/* Function io_create() */
/* ==================================================================================================== */
static int io_create (bool disown, uint64_t eventID, uint64_t eventMask, int64_t offset, uint64_t flags, int64_t level, bool offset_negative, bool translate_mask)
{
  /* Helpers */
  bool   io_found          = false;
  bool   io_edge           = false;
  bool   io_AcceptConflict = false;
  bool   io_AcceptDelayed  = false;
  bool   io_AcceptEarly    = false;
  bool   io_AcceptLate     = false;
  int64_t io_offset         = offset;

  /* Get level/edge */
  if (level > 0) { io_edge = true; }
  else           { io_edge = false; }

  /* Perform selected action(s) */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< std::string, std::string > outs;
    std::string io_path;
    outs = receiver->getOutputs();
    std::shared_ptr<Output_Proxy> output_proxy;

    /* Check flags */
    if (flags & (1 << ECA_LATE))     { io_AcceptLate = true; }
    if (flags & (1 << ECA_EARLY))    { io_AcceptEarly = true; }
    if (flags & (1 << ECA_CONFLICT)) { io_AcceptConflict = true; }
    if (flags & (1 << ECA_DELAYED))  { io_AcceptDelayed = true; }

    /* Check if a negative offset is wanted */
    if (offset_negative) { io_offset = io_offset*-1; }

    /* Check if IO exists output */
    if (ioNameGiven)
    {
      for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
      {
        if (it->first == ioName)
        {
          io_found = true;
          output_proxy = Output_Proxy::create(it->second);
          io_path = it->second;
        }
      }
    }

    /* Found IO? */
    if (ioNameGiven == false)
    {
      std::cerr << "Error: No IO given!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }
    else if (io_found == false)
    {
      std::cerr << "Error: There is no IO (output) with the name " << ioName << "!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }

    /* Setup condition */
    std::shared_ptr<OutputCondition_Proxy> condition;
    if (translate_mask) { condition = OutputCondition_Proxy::create(output_proxy->NewCondition(true, eventID, tr_mask(eventMask), io_offset, io_edge)); }
    else                { condition = OutputCondition_Proxy::create(output_proxy->NewCondition(true, eventID, eventMask, io_offset, io_edge)); }
    condition->setAcceptConflict(io_AcceptConflict);
    condition->setAcceptDelayed(io_AcceptDelayed);
    condition->setAcceptEarly(io_AcceptEarly);
    condition->setAcceptLate(io_AcceptLate);

    /* Disown and quit or keep waiting */
    if (disown) { condition->Disown(); }
    else        { std::cout << "Condition created..." << std::endl;
      while(true) {
        saftlib::wait_for_signal();
      }
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_destroy() */
/* ==================================================================================================== */
static int io_destroy (bool verbose_mode)
{
  /* Helper */
  std::string c_name = "Unknown";
  std::vector <std::shared_ptr<OutputCondition_Proxy> > prox;

  /* Perform selected action(s) */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< std::string, std::string > outs;
    std::string io_path;
    outs = receiver->getOutputs();
    std::shared_ptr<Output_Proxy> output_proxy;

    /* Check if IO exists output */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      output_proxy = Output_Proxy::create(it->second);
      std::vector< std::string > all_conditions = output_proxy->getAllConditions();
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          std::shared_ptr<OutputCondition_Proxy> destroy_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
          c_name = all_conditions[condition_it];
          if (destroy_condition->getDestructible() && (destroy_condition->getOwner() == ""))
          {
            prox.push_back ( OutputCondition_Proxy::create(all_conditions[condition_it]));
            prox.back()->Destroy();
            if (verbose_mode) { std::cout << "Destroyed " << c_name << "!" << std::endl; }
          }
          else
          {
            if (verbose_mode) { std::cout << "Found " << c_name << " but is not destructible!" << std::endl; }
          }
        }
      }
    }

  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_flip() */
/* ==================================================================================================== */
static int io_flip (bool verbose_mode)
{
  /* Helper */
  std::string c_name = "Unknown";

  /* Perform selected action(s) */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< std::string, std::string > outs;
    std::string io_path;
    outs = receiver->getOutputs();
    std::shared_ptr<Output_Proxy> output_proxy;
    std::vector< std::string > conditions_to_destroy;

    /* Check if IO exists output */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      output_proxy = Output_Proxy::create(it->second);
      std::vector< std::string > all_conditions = output_proxy->getAllConditions();
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          std::shared_ptr<OutputCondition_Proxy> flip_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
          c_name = all_conditions[condition_it];

          /* Flip unowned conditions */
          if ((flip_condition->getOwner() == ""))
          {
            if (verbose_mode) { std::cout << "Flipped " << c_name; }
            if (flip_condition->getActive())
            {
              flip_condition->setActive(false);
              if (verbose_mode) { std::cout << " to inactive!" << std::endl; }
            }
            else
            {
              flip_condition->setActive(true);
              if (verbose_mode) { std::cout << " to active!" << std::endl; }
            }
          }
          else
          {
            if (verbose_mode) { std::cout << "Found " << c_name << " but is already owned!" << std::endl; }
          }
        }
      }
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_list() */
/* ==================================================================================================== */
static int io_list (void)
{
  /* Helpers */
  bool     io_found     = false;
  bool     header_shown = false;
  std::string c_name  = "Unknown";

  /* Perform selected action(s) */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< std::string, std::string > outs;
    std::string io_path;
    outs = receiver->getOutputs();
    std::shared_ptr<Output_Proxy> output_proxy;

    /* Check if IO exists output */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      output_proxy = Output_Proxy::create(it->second);

      if(!header_shown)
      {
        /* Print table header */
        std::cout << "IO             Number      ID                  Mask                Offset      Flags       Edge     Status    Owner   " << std::endl;
        std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
        header_shown = true;
      }

      if (ioNameGiven && (it->first == ioName)) { io_found = true; }
      std::vector< std::string > all_conditions = output_proxy->getAllConditions();

      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          /* Helper for flag information field */
          uint32_t  flags = 0x0;

          /* Get output conditions */
          std::shared_ptr<OutputCondition_Proxy> info_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
          c_name = all_conditions[condition_it];
          std::string str_path_and_id = it->first;
          std::string str_path        = it->second;
          std::string cid             = c_name;
          std::string cid_prefix      = "/_";

          /* Extract IO name */
          cid.replace(str_path.find(str_path),str_path.length(),"");
          cid.replace(cid_prefix.find(cid_prefix),cid_prefix.length(),"");

          /* Output ECA condition table */
          std::cout << std::left;
          std::cout << std::setw(12+2) << it->first << " ";
          std::cout << std::setw(10+1) << cid << " ";
          std::cout << "0x";
          std::cout << std::setw(16+1) << std::hex << info_condition->getID() << " ";
          std::cout << "0x";
          std::cout << std::setw(16+1) << std::hex << info_condition->getMask() << " ";
          std::cout << std::dec;
          std::cout << std::setw(10+1) << info_condition->getOffset() << " ";
          if (info_condition->getAcceptDelayed())  { std::cout << "d"; flags = flags | (1<<ECA_DELAYED); }
          else                                     { std::cout << "."; }
          if (info_condition->getAcceptConflict()) { std::cout << "c"; flags = flags | (1<<ECA_CONFLICT); }
          else                                     { std::cout << "."; }
          if (info_condition->getAcceptEarly())    { std::cout << "e"; flags = flags | (1<<ECA_EARLY); }
          else                                     { std::cout << "."; }
          if (info_condition->getAcceptLate())     { std::cout << "l"; flags = flags | (1<<ECA_LATE); }
          else                                     { std::cout << "."; }
          std::cout << " (0x";
          std::cout << std::setw(1) << std::hex << flags << ")  ";
          if (info_condition->getOn())             { std::cout << "Rising   "; }
          else                                     { std::cout << "Falling  "; }
          if (info_condition->getActive())         { std::cout << "Active    "; }
          else                                     { std::cout << "Inactive  "; }
          std::cout << std::dec;
          std::cout << info_condition->getOwner() << " ";
          std::cout << std::endl;
        }
      }
    }

    /* Found IO? */
    if ((io_found == false) &&(ioNameGiven))
    {
      std::cout << "Error: There is no IO with the name " << ioName << "!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }

  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_help() */
/* ==================================================================================================== */
static void io_help (void)
{
  /* Print arguments and options */
  std::cout << "burst control for SAFTlib " << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -n <name>:                                     Specify IO name or leave blank (to see all IOs/conditions)" << std::endl;
  std::cout << std::endl;
  std::cout << "  -A:                                            Get the firmware id of the burst generator" << std::endl;
  std::cout << "  -N <id>                                        Specify burst ID" << std::endl;
  std::cout << "      id                                           Burst id, 1..32" << std::endl;
  std::cout << "  -S <trigger> <mask>                            Create new burst. Requires -n and -N options!" << std::endl;
//  std::cout << "  -T <trigger> <toggle> <mask>                   Create new toggling burst. Requires -n and -N options!" << std::endl;
  std::cout << "      trigger, toggle, mask                        IDs and mask for triggering and toggling events" << std::endl;
  std::cout << "  -B <t_hi> <t_p> <b_p> <b_d> <b_f>:             Define signal parameters to a new/existing burst. Requires -N option!" << std::endl;
  std::cout << "      t_hi, t_p                                    Signal high width (ns), signal period (ns)" << std::endl;
  std::cout << "      b_p, b_d, b_f                                Burst period (=0 endless), burst delay (ns), burst flag (=0 new/overwrite, 1=append)" << std::endl;
  std::cout << "  -L <0 | id>                                    List burst(s): 0 for burst IDs, otherwise burst info" << std::endl;
  std::cout << "  -E <id> <disen>                                Dis/enable burst(s): disable if disen = 0, otherwise enable" << std::endl;
  std::cout << "  -R <id>                                        Remove burst(s)" << std::endl;
  std::cout << std::endl;
/*  std::cout << "  -I <instr_code> [u32 u32 ...]:                 Instruction to the burst generator, allowed instructions are listed below:" << std::endl;
  std::cout << std::endl;
  std::cout << "      0x1                                        Print the burst parameters. Arguments: burst_id" << std::endl;
  std::cout << "      0x2                                        Obtain the burst parameters. Arguments: burst_id, delay, conditions, block period, flag, verbose" << std::endl;
  std::cout << "      0x3                                        Obtain the production cycles. Arguments: burst_id, cycle_h32, cycle_l32, verbose" << std::endl;
  std::cout << "      0x10                                       Print MSI configuration" << std::endl;
  std::cout << "      0x11                                       Print ECA channel counters" << std::endl;
  std::cout << "      0x12                                       Print ECA queue content" << std::endl;
  std::cout << "  All print instructions require the eb-console tool to be run to see the output" << std::endl;*/
  std::cout << "  -X                                             Clear all unowned conditions for the IO and eCPU actions." << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl << std::endl;
  std::cout << program << " tr0" << " -L 0" << std::endl;
  std::cout << "   List the IDs of all bursts." << std::endl << std::endl;
  std::cout << program << " tr0" << " -L 1 -v" << std::endl;
  std::cout << "   Show the info of a burst with ID 1. It includes IO port name, IDs of trigger and toggle events, enable state." << std::endl << std::endl;
  std::cout << program << " tr0" << " -n B1" << " -N 1" << " -S 0x0000991000000000 0xffffffff00000000" << std::endl;
  std::cout << "   Create new burst at the output port B1. The burst ID is 1 and it is triggered by a timing message with the given event ID." << std::endl << std::endl;
  std::cout << program << " tr0" << " -N 1" << " -B 1000000 2000000 4000000 0 0" << std::endl;
  std::cout << "   Define signal for the burst with ID 1. The given burst must have already been created! Signal high width is 1 ms, signal period is 2 ms, burst length is 4 ms, no delay." << std::endl << std::endl;
  std::cout << program << " tr0" << " -E 1 1" << std::endl;
  std::cout << "   Enable the burst with ID 1." << std::endl << std::endl;
  std::cout << program << " tr0" << " -R 1" << std::endl;
  std::cout << "   Remove the burst with ID 1." << std::endl;
  std::cout << std::endl;
/*  std::cout << "  -o 0/1:                                        Toggle output enable" << std::endl;
  std::cout << "  -t 0/1:                                        Toggle termination/resistor" << std::endl;
  std::cout << "  -q 0/1:                                        Toggle special (output) functionality" << std::endl;
  std::cout << "  -e 0/1:                                        Toggle special (input) functionality" << std::endl;
  std::cout << "  -m 0/1:                                        Toggle BuTiS t0 + TS gate/mux" << std::endl;
  std::cout << "  -p 0/1:                                        Toggle WR PPS gate/mux" << std::endl;
  std::cout << "  -a 0/1:                                        Toggle gate (output)" << std::endl;
  std::cout << "  -j 0/1:                                        Toggle gate (input)" << std::endl;
  std::cout << "  -d 0/1:                                        Drive output value" << std::endl;
  std::cout << "  -k <time [ns]>                                 Change stable time" << std::endl;
  std::cout << std::endl;
  std::cout << "  -i:                                            List every IO and it's capability" << std::endl;
  std::cout << std::endl;
  std::cout << "  -s:                                            Snoop on input(s)" << std::endl;
  std::cout << "  -w:                                            Disable all events from input(s)" << std::endl;
  std::cout << "  -y:                                            Display IO input to event table" << std::endl;
  std::cout << "  -b <prefix>:                                   Enable event source(s) and set prefix" << std::endl;
  std::cout << "  -r:                                            Disable event source(s)" << std::endl;
  std::cout << std::endl;
  std::cout << "  -c <event id> <mask> <offset> <flags> <level>: Create a new condition (active)" << std::endl;
  std::cout << "  -g:                                            Negative offset (new condition)" << std::endl;
  std::cout << "  -u:                                            Disown the created condition" << std::endl;
  std::cout << "  -x:                                            Destroy all unowned conditions" << std::endl;
  std::cout << "  -f:                                            Flip active/inactive conditions" << std::endl;
  std::cout << "  -z:                                            Translate mask" << std::endl;
  std::cout << "  -l:                                            List all conditions" << std::endl;
  std::cout << std::endl;
  std::cout << "  -v:                                            Switch to verbose mode" << std::endl;
  std::cout << "  -h:                                            Print help (this message)" << std::endl;
  std::cout << std::endl;
  std::cout << "Condition <flags> parameter:" << std::endl;
  std::cout << "  Accept Late:                                   0x1 (l)" << std::endl;
  std::cout << "  Accept Early:                                  0x2 (e)" << std::endl;
  std::cout << "  Accept Conflict:                               0x4 (c)" << std::endl;
  std::cout << "  Accept Delayed:                                0x8 (d)" << std::endl;
  std::cout << std::endl;
  std::cout << "  These flags can be used in combination (e.g. flag 0x3 will accept late and early events)" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << program << " exploder5a_123t " << "-n IO1 " << "-o 1 -t 0 -d 1" << std::endl;
  std::cout << "  This will enable the output and disable the input termination and drive the output high" << std::endl;
  std::cout << std::endl;*/
  std::cout << "Report bugs to <csco-tg@gsi.de>" << std::endl;
  std::cout << "Licensed under the GPLv3" << std::endl;
  std::cout << std::endl;
}

/* Function io_list_i_to_e() */
/* ==================================================================================================== */
static int io_list_i_to_e()
{
  /* Helper  */
  bool printed_header = false;

  /* Get inputs and snoop */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > inputs = receiver->getInputs();

    /* Check inputs */
    for (std::map<std::string,std::string>::iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {

      if (!printed_header)
      {
        std::cout << "IO             Prefix              Enabled  Event Bit(s)" << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
        printed_header = true;
      }
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        /* Set name */
        ioName =  it->first.c_str();
        uint64_t prefix = ECA_EVENT_ID_LATCH + (map_PrefixName.size()*2);
        map_PrefixName[ioName] = prefix;
        std::shared_ptr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);

        std::cout << std::left;
        std::cout << std::setw(12+2) << it->first << " ";
        std::cout << "0x";
        std::cout << std::setw(16+1) << std::hex <<  input->getEventPrefix() << " " << std::dec;
        std::cout << std::setw(3+2);
        if (input->getEventEnable()) { std::cout << "Yes"; }
        else                         { std::cout << "No "; }
        std::cout << "    ";
        std::cout << "0x";
        std::cout << std::setw(10+1) << std::hex <<  input->getEventBits() << " " << std::dec;
        std::cout << std::endl;
      }
    }
  }
  catch (const saftbus::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_catch_input() */
/* ==================================================================================================== */
static void io_catch_input(uint64_t event, uint64_t param, uint64_t deadline, uint64_t executed, uint16_t flags)
{
  /* Helpers */
  uint64_t time = deadline - IO_CONDITION_OFFSET;
  std::string catched_io = "Unknown";

  /* !!! evaluate prefix<>name map */
  for (std::map<std::string,uint64_t>::iterator it=map_PrefixName.begin(); it!=map_PrefixName.end(); ++it) { if (event == it->second) { catched_io = it->first; } } /* Rising */
  for (std::map<std::string,uint64_t>::iterator it=map_PrefixName.begin(); it!=map_PrefixName.end(); ++it) { if (event-1 == it->second) { catched_io = it->first; } } /* Falling */

  /* Format output */
  std::cout << std::left;
  std::cout << std::setw(12+2) << catched_io << " ";
  if ((event&1)) { std::cout << "Rising   "; }
  else           { std::cout << "Falling  "; }
  if (flags & (1 << ECA_DELAYED))  { std::cout << "d"; }
  else                             { std::cout << "."; }
  if (flags & (1 << ECA_CONFLICT)) { std::cout << "c"; }
  else                             { std::cout << "."; }
  if (flags & (1 << ECA_EARLY))    { std::cout << "e"; }
  else                             { std::cout << "."; }
  if (flags & (1 << ECA_LATE))     { std::cout << "l"; }
  else                             { std::cout << "."; }
  std::cout << " (0x";
  std::cout << std::setw(1) << std::hex << flags << ")  ";
  std::cout << "0x" << std::hex << setw(16+1) << event << std::dec << " ";
  std::cout << "0x" << std::hex << setw(16+1) << time << std::dec << " " << tr_formatDate(time,PMODE_VERBOSE);
  std::cout << std::endl;
}

/* Function io_snoop() */
/* ==================================================================================================== */
static int io_snoop(bool mode, bool setup_only, bool disable_source, uint64_t prefix_custom)
{
  /* Helpers (connect proxies in a vector) */
  std::vector <std::shared_ptr<SoftwareCondition_Proxy> > proxies;
  std::vector <std::shared_ptr<SoftwareActionSink_Proxy> > sinks;

  /* Get inputs and snoop */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > inputs = receiver->getInputs();

    /* Check inputs */
    for (std::map<std::string,std::string>::iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        /* Set name */
        ioName =  it->first.c_str();
        uint64_t prefix = ECA_EVENT_ID_LATCH + (map_PrefixName.size()*2);
        map_PrefixName[ioName] = prefix;

        /* Create sink and condition or turn off event source */
        if (!mode)
        {
          if (!setup_only)
          {
            sinks.push_back( SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink("")));
            proxies.push_back( SoftwareCondition_Proxy::create(sinks.back()->NewCondition(true, prefix, -2, IO_CONDITION_OFFSET)));
            proxies.back()->Action.connect(sigc::ptr_fun(&io_catch_input));
            proxies.back()->setAcceptConflict(true);
            proxies.back()->setAcceptDelayed(true);
            proxies.back()->setAcceptEarly(true);
            proxies.back()->setAcceptLate(true);
          }

          /* Setup the event */
          std::shared_ptr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);
          if (prefix_custom != 0x0) { prefix = prefix_custom; }
          if (disable_source)
          {
            input->setEventEnable(false);
          }
          else
          {
            input->setEventEnable(false);
            input->setEventPrefix(prefix);
            input->setEventEnable(true);
          }
        }
        else
        {
          /* Disable the event */
          std::shared_ptr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);
          input->setEventEnable(false);
        }
      }
    }

    /* Disabled? */
    if (mode) { return (__IO_RETURN_SUCCESS); }

    /* Run the loop printing IO events (in case we found inputs */
    if (map_PrefixName.size())
    {
      if (!setup_only)
      {
        std::cout << "IO             Edge     Flags       ID                  Timestamp           Formatted Date               " << std::endl;
        std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;
        while(true) {
          saftlib::wait_for_signal();
        }
      }
    }
    else
    {
      std::cout << "This Timing Receiver has no inputs!" << std::endl;
    }

  }
  catch (const saftbus::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_setup() */
/* ==================================================================================================== */
static int io_setup (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_gate_out, int io_gate_in, int io_mux, int io_pps, int io_drive, int stime,
                    bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_gate_out, bool set_gate_in, bool set_mux, bool set_pps, bool set_drive, bool set_stime,
                    bool verbose_mode)
{
  unsigned io_type  = 0;     /* Out, Inout or In? */
  bool     io_found = false; /* IO exists? */
  bool     io_set   = false; /* IO set or get configuration */

  /* Check if there is at least one parameter to set */
  io_set = set_oe | set_term | set_spec_out | set_spec_in | set_mux | set_pps | set_drive | set_stime | set_gate_out | set_gate_in;

  /* Display information */
  if (verbose_mode)
  {
    if (io_set) { std::cout << "Checking configuration feasibility..." << std::endl; }
    else        { std::cout << "Checking current configuration..." << std::endl; }
  }

  /* Plausibility check for io name */
  if (ioName == NULL)
  {
    std::cout << "Error: No IO name provided!" << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Initialize saftlib components */

  /* Perform selected action(s) */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< std::string, std::string > outs;
    std::map< std::string, std::string > ins;
    std::string io_path;
    std::string io_partner;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    std::shared_ptr<Output_Proxy> output_proxy;
    std::shared_ptr<Input_Proxy> input_proxy;

    /* Check if IO exists as input */
    for (std::map<std::string,std::string>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      if (it->first == ioName)
      {
        io_found = true;
        io_type = IO_CFG_FIELD_DIR_INPUT;
        input_proxy = Input_Proxy::create(it->second);
        io_path = it->second;
      }
    }

    /* Check if IO exists output */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (it->first == ioName)
      {
        io_found = true;
        io_type = IO_CFG_FIELD_DIR_OUTPUT;
        output_proxy = Output_Proxy::create(it->second);
        io_path = it->second;
      }
    }

    /* Found IO? */
    if (io_found == false)
    {
      std::cout << "Error: There is no IO with the name " << ioName << "!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }
    else
    {
      if (verbose_mode) { std::cout << "Info: Found " << ioName << " (" << io_type << ")" << "!" << std::endl; }
    }

    /* Check if IO is bidirectional */
    if (io_type == IO_CFG_FIELD_DIR_INPUT)
    {
      io_partner = input_proxy->getOutput();
      if (io_partner != "") { io_type = IO_CFG_FIELD_DIR_INOUT; }
    }
    else if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
    {
      io_partner = output_proxy->getInput();
      if (io_partner != "") { io_type = IO_CFG_FIELD_DIR_INOUT; }
    }
    else
    {
      std::cout << "IO direction is unknown!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }

    /* Switch: Set something or get the current status) ?*/
    if (io_set)
    {
      /* Check oe configuration */
      if (set_oe)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }

        /* Check if OE option is available */
        if (!(output_proxy->getOutputEnableAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check term configuration */
      if (set_term)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
        /* Check if TERM option is available */
        if (!(input_proxy->getInputTerminationAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check special out configuration */
      if (set_spec_out)
      {
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
        /* Check if SPEC OUT option is available */
        if (!(output_proxy->getSpecialPurposeOutAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check special in configuration */
      if (set_spec_in)
      {
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
        /* Check if SPEC OUT option is available */
        if (!(input_proxy->getSpecialPurposeInAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check gate out configuration */
      if (set_gate_out)
      {
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check gate in configuration */
      if (set_gate_in)
      {
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check multiplexer (BuTiS support) */
      if (set_mux)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check PPS gate */
      if (set_pps)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check if IO can be driven */
      if (set_drive)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Check if stable time can beset */
      if (set_stime)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl;
          return (__IO_RETURN_FAILURE);
        }
      }

      /* Set configuration */
      if (set_oe)       { output_proxy->setOutputEnable(io_oe); }
      if (set_term)     { input_proxy->setInputTermination(io_term); }
      if (set_spec_out) { output_proxy->setSpecialPurposeOut(io_spec_out); }
      if (set_spec_in)  { input_proxy->setSpecialPurposeIn(io_spec_in); }
      if (set_gate_out) { output_proxy->setGateOut(io_gate_out); }
      if (set_gate_in)  { input_proxy->setGateIn(io_gate_in); }
      if (set_mux)      { output_proxy->setBuTiSMultiplexer(io_mux); }
      if (set_pps)      { output_proxy->setPPSMultiplexer(io_pps); }
      if (set_drive)    { output_proxy->WriteOutput(io_drive); }
      if (set_stime)    { input_proxy->setStableTime(stime); }
    }
    else
    {
      /* Generic information */
      std::cout << "IO:" << std::endl;
      std::cout << "  Path:    " << io_path << std::endl;
      if (io_partner != "") { std::cout << "  Partner: " << io_partner << std::endl; }
      std::cout << std::endl;

      /* Display configuration */
      std::cout << "Current state:" << std::endl;
      if (io_type != IO_CFG_FIELD_DIR_INPUT)
      {
        /* Display output */
        if (output_proxy->ReadOutput())             { std::cout << "  Output:           High" << std::endl; }
        else                                        { std::cout << "  Output:           Low" << std::endl; }

        /* Display output enable state */
        if (output_proxy->getOutputEnableAvailable())
        {
          if (output_proxy->getOutputEnable())      { std::cout << "  OutputEnable:     On" << std::endl; }
          else                                      { std::cout << "  OutputEnable:     Off" << std::endl; }
        }

        /* Display special out state */
        if (output_proxy->getSpecialPurposeOutAvailable())
        {
          if (output_proxy->getSpecialPurposeOut()) { std::cout << "  SpecialOut:       On" << std::endl; }
          else                                      { std::cout << "  SpecialOut:       Off" << std::endl; }
        }

        /* Display gate out state */
        if (output_proxy->getGateOut())             { std::cout << "  GateOut:          On" << std::endl; }
        else                                        { std::cout << "  GateOut:          Off" << std::endl; }

        /* Display BuTiS multiplexer state */
        if (output_proxy->getBuTiSMultiplexer())    { std::cout << "  BuTiS t0 + TS:    On" << std::endl; }
        else                                        { std::cout << "  BuTiS t0 + TS:    Off" << std::endl; }

        /* Display WR PPS multiplexer state */
        if (output_proxy->getPPSMultiplexer())      { std::cout << "  WR PPS:           On" << std::endl; }
        else                                        { std::cout << "  WR PPS:           Off" << std::endl; }

        /* Display IO index */
        std::cout << "  IndexOut:         " << output_proxy->getIndexOut() << std::endl;

      }

      if (io_type != IO_CFG_FIELD_DIR_OUTPUT)
      {
        /* Display input */
        if (input_proxy->ReadInput())               { std::cout << "  Input:            High" << std::endl; }
        else                                        { std::cout << "  Input:            Low" << std::endl; }

        /* Display output enable state */
        if (input_proxy->getInputTerminationAvailable())
        {
          if (input_proxy->getInputTermination())   { std::cout << "  InputTermination: On" << std::endl; }
          else                                      { std::cout << "  InputTermination: Off" << std::endl; }
        }

        /* Display special in state */
        if (input_proxy->getSpecialPurposeInAvailable())
        {
          if (input_proxy->getSpecialPurposeIn())   { std::cout << "  SpecialIn:        On" << std::endl; }
          else                                      { std::cout << "  SpecialIn:        Off" << std::endl; }
        }

        /* Display gate in state */
        if (input_proxy->getGateIn())               { std::cout << "  GateIn:           On" << std::endl; }
        else                                        { std::cout << "  GateIn:           Off" << std::endl; }

        /* Display stable time */
        std::cout << "  StableTime:       " << input_proxy->getStableTime() << " ns" << std::endl;

        /* Display IO index */
        std::cout << "  IndexIn:          " << input_proxy->getIndexIn() << std::endl;
      }
    }
  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_print_table() */
/* ==================================================================================================== */
static int io_print_table(bool verbose_mode)
{
  /* Initialize saftlib components */

  /* Try to get the table */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > outs;
    std::map< std::string, std::string > ins;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();

    /* Show verbose output */
    if (verbose_mode)
    {
      std::cout << "Discovered Output(s): " << std::endl;
      for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
      std::cout << "Discovered Inputs(s) " << std::endl;
      for (std::map<std::string,std::string>::iterator it=ins.begin(); it!=ins.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
    }

    /* Print table header */
    std::cout << "Name           Direction  OutputEnable  InputTermination  SpecialOut  SpecialIn  Resolution  Logic Level" << std::endl;
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

    /* Print Outputs */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        std::shared_ptr<Output_Proxy> output_proxy;
        output_proxy = Output_Proxy::create(it->second);
        std::cout << std::left;
        std::cout << std::setw(12+2) << it->first << " ";
        std::cout << std::setw(5+6)  << "Out ";
        std::cout << std::setw(3+11);
        if(output_proxy->getOutputEnableAvailable()) { std::cout << "Yes"; }
        else                                         { std::cout << "No"; }
        std::cout << std::setw(3+15);
        std::cout << "No"; /* InputTermination */
        std::cout << std::setw(5+7);
        if(output_proxy->getSpecialPurposeOutAvailable()) { std::cout << "Yes"; }
        else                                              { std::cout << "No"; }
        std::cout << std::setw(3+8);
        std::cout << "No"; /* SpecialOut */
        std::cout << output_proxy->getTypeOut();
        std::cout << "  ";
        std::cout << output_proxy->getLogicLevelOut();
        std::cout << std::endl;
      }
    }

    /* Print Inputs */
    for (std::map<std::string,std::string>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        std::shared_ptr<Input_Proxy> input_proxy;
        input_proxy = Input_Proxy::create(it->second);
        std::cout << std::left;
        std::cout << std::setw(12+2) << it->first << " ";
        std::cout << std::setw(5+6)  << "In ";
        std::cout << std::setw(3+11);
        std::cout << "No";
        std::cout << std::setw(3+15);
        if(input_proxy->getInputTerminationAvailable()) { std::cout << "Yes"; }
        else                                            { std::cout << "No"; }
        std::cout << std::setw(5+7);
        std::cout << "No";
        std::cout << std::setw(3+8);
        if(input_proxy->getSpecialPurposeInAvailable()) { std::cout << "Yes"; }
        else                                            { std::cout << "No"; }
        std::cout << input_proxy->getTypeIn();
        std::cout << "  ";
        std::cout << input_proxy->getLogicLevelIn();
        std::cout << std::endl;
      }
    }

  }
  catch (const saftbus::Error& error)
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function main() */
/* ==================================================================================================== */
int main (int argc, char** argv)
{
  /* Helpers to deal with endless arguments */
  char * pEnd         = NULL;  /* Arguments parsing */
  int  opt            = 0;     /* Number of given options */
  int  io_oe          = 0;     /* Output enable */
  int  io_term        = 0;     /* Input Termination */
  int  io_spec_out    = 0;     /* Special (output) function */
  int  io_spec_in     = 0;     /* Special (input) function */
  int  io_gate_out    = 0;     /* Output gate */
  int  io_gate_in     = 0;     /* Input gate */
  int  io_mux         = 0;     /* Gate (BuTiS) */
  int  io_pps         = 0;     /* Gate (PPS) */
  int  io_drive       = 0;     /* Drive IO value */
  int  ioc_flip       = 0;     /* Flip active bit for all conditions */
  int  stime          = 0;     /* Stable time to set */
  int  return_code    = 0;     /* Return code */
  uint64_t eventID     = 0x0;   /* Event ID (new condition) */
  uint64_t eventMask   = 0x0;   /* Event mask (new condition) */
  int64_t  offset      = 0x0;   /* Event offset (new condition) */
  uint64_t flags       = 0x0;   /* Accept flags (new condition) */
  int64_t  level       = 0x0;   /* Rising or falling edge (new condition) */
  uint64_t prefix      = 0x0;   /* IO input prefix */
  bool translate_mask = false; /* Translate mask? */
  bool negative       = false; /* Offset negative? */
  bool ioc_valid      = false; /* Create arguments valid? */
  bool set_oe         = false; /* Set? */
  bool set_term       = false; /* Set? */
  bool set_spec_in    = false; /* Set? */
  bool set_spec_out   = false; /* Set? */
  bool set_gate_in    = false; /* Set? */
  bool set_gate_out   = false; /* Set? */
  bool set_mux        = false; /* Set? (BuTiS gate) */
  bool set_pps        = false; /* Set? (PPS gate) */
  bool set_drive      = false; /* Set? */
  bool set_stime      = false; /* Set? (Stable time) */
  bool ios_snoop      = false; /* Snoop on an input(s) */
  bool ios_wipe       = false; /* Wipe/disable all events from input(s) */
  bool ios_i_to_e     = false; /* List input to event table */
  bool ios_setup_only = false; /* Only setup input to event */
  bool ioc_create     = false; /* Create condition */
  bool ioc_disown     = false; /* Disown created condition */
  bool ioc_destroy    = false; /* Destroy condition */
  bool ioc_list       = false; /* List conditions */
  bool ioc_dis_ios    = false; /* Disable event source? */
  bool verbose_mode   = false; /* Print verbose output to output stream => -v */
  bool show_help      = false; /* Print help => -h */
  bool show_table     = false; /* Print io mapping table => -i */

  uint64_t bg_e_id    = 0x00;  /* Event ID */
  uint64_t bg_e_mask  = 0x00;  /* Event mask */
  uint32_t bg_t_high  = 0x00;  /* Signal active state length, t_high */
  uint32_t bg_t_p     = 0x00;  /* Signal period, t_p */
  int64_t  bg_t_b     = 0x00;  /* Burst period, t_b (=0 endless) */
  uint64_t bg_b_delay = 0x00;  /* Burst delay, b_d, nanoseconds */
  uint32_t bg_b_flag  = 0x00;  /* Burst flag, b_f, (=0 new/overwrite, =1 append) */
  int      bg_disen   = 0;     /* Disable or enable option argument value */
  std::vector<std::string> bg_instr_args;  /* Arguments of instruct option */
  int      bg_id      = 0x00;  /* Burst ID */
  bool bg_fw_id       = false; /* Get and print the firmware id of the burst generator */
  bool bg_instr       = false; /* Demo option to test the instruct method call */
  bool bg_cfg_io      = false; /* Set the ECA conditions for IO actions */
  bool bg_clr_all     = false; /* Clear all unowned conditions for the IO and eCPU actions */
  bool bg_ls_bursts   = false; /* List enabled bursts */
  bool bg_disen_burst = false; /* Disable or enable burst, id */
  bool bg_mk_burst    = false; /* Create new burst, id, trigger e_id, toggle e_id, e_mask */
  bool bg_rm_burst    = false; /* Remove burst */

  /* Get the application name */
  program = argv[0];
  deviceName = "NoDeviceNameGiven";

  /* Parse for options */
  while ((opt = getopt(argc, argv, ":AB:E:I:L:N:R:S:T:X!:%:n:o:t:q:e:m:p:d:k:swyb:rc:guxfzlivh")) != -1)
  {
    switch (opt)
    {
      case 'A': { bg_fw_id       = true; break; }
      case 'B': {
                  if (argv[optind-1] != NULL) { bg_t_high = strtoul(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error::Missing active state length of a pulse (u32)!" << std::endl; return -1; }
                  if (argv[optind+0] != NULL) { bg_t_p = strtoul(argv[optind+0], &pEnd, 0); }
                  else                        { std::cerr << "Error::Missing signal period (u32)!" << std::endl; return -1; }
                  if (argv[optind+1] != NULL) { bg_t_b = strtoll(argv[optind+1], &pEnd, 0); }
                  else                        { std::cerr << "Error::Missing burst period (i64)!" << std::endl; return -1; }
                  if (bg_t_high > bg_t_p)    { std::cerr << "Invalid signal parameters, t_high > t_p" << std::endl; return -1; }
                  if (bg_t_b > 0 && bg_t_p > bg_t_b) { std::cerr << "Invalid burst parameters, t_p > t_b" << std::endl; return -1; }
                  if (argv[optind+2] != NULL) { bg_b_delay = strtoull(argv[optind+2], &pEnd, 0); }
                  else                        { std::cerr << "Error::Missing burst delay!" << std::endl; return -1; }
                  if (argv[optind+3] != NULL) { bg_b_flag = strtoul(argv[optind+3], &pEnd, 0); }
                  else                        { std::cerr << "Error::Missing burst flag!" << std::endl; return -1; }
                  bg_cfg_io      = true;
                  break; }
      case 'E': { if (argv[optind-1] != NULL) { bg_id = strtol(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing burst id!" << std::endl; return -1; }
                  if (bg_id < 0 || bg_id > N_BURSTS) { std::cerr << "Error: invalid burst id, must be 0 <= id <= " << N_BURSTS << std::endl; return -1; }
                  if (argv[optind+0] != NULL) { bg_disen = strtol(argv[optind+0], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing dis/enable setting!" << std::endl; return -1; }
                  bg_disen_burst = true;
                  break; }
      case 'I': { bg_instr       = true;
                  int index      = optind - 1;
                  while (index < argc)
                  {
                    if (argv[index] != NULL && argv[index][0] != '-') { bg_instr_args.push_back(argv[index]); ++index; }
                    else                                              { optind = index - 1; break; }
                  }
                  if (!bg_instr_args.empty())
                  {
                    std::cout << "Arguments: ";
                    for (vector<string>::iterator it = bg_instr_args.begin(); it != bg_instr_args.end(); ++it)
                      std::cout << *it << ' ';
                    std::cout << std::endl;
                  }
                  break; }
      case 'L': { if (argv[optind-1] != NULL) { bg_id = strtol(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing burst id!" << std::endl; return -1; }
                  if (bg_id < 0 || bg_id > N_BURSTS)  { std::cerr << "Error: invalid burst id, must be 0 < id <= " << N_BURSTS << std::endl; return -1; }
                  bg_ls_bursts   = true;
                  break; }
      case 'N': { if (argv[optind-1] != NULL) { burstId = strtol(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing burst id!" << std::endl; return -1; }
                  if (burstId <= 0 || burstId > N_BURSTS)  { std::cerr << "Error: invalid burst id, must be 0 < id <= " << N_BURSTS << std::endl; return -1; }
                  burstIdGiven  = true;
                  break; }
      case 'R': { if (argv[optind-1] != NULL) { bg_id = strtol(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing burst id!" << std::endl; return -1; }
                  if (bg_id <= 0 || bg_id > N_BURSTS) { std::cerr << "Error: invalid burst id, must be 0 < id <= " << N_BURSTS << std::endl; return -1; }
                  bg_rm_burst   = true;
                  break; }
      case 'S': { if (argv[optind-1] != NULL) { bg_e_id = strtoull(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing trigger event id!" << std::endl; return -1; }
                  if (argv[optind+0] != NULL) { bg_e_mask = strtoull(argv[optind+0], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing trigger event mask!" << std::endl; return -1; }
                  bg_mk_burst   = true;
                  break; }
      case 'X': { bg_clr_all    = true;
                  break; }
      case 'n': { ioName         = argv[optind-1]; ioNameGiven  = true; break; }
      case 'o': { io_oe          = atoi(optarg);   set_oe       = true; break; }
      case 't': { io_term        = atoi(optarg);   set_term     = true; break; }
      case 'q': { io_spec_out    = atoi(optarg);   set_spec_out = true; break; }
      case 'e': { io_spec_in     = atoi(optarg);   set_spec_in  = true; break; }
      case 'a': { io_gate_out    = atoi(optarg);   set_gate_out = true; break; }
      case 'j': { io_gate_in     = atoi(optarg);   set_gate_in  = true; break; }
      case 'm': { io_mux         = atoi(optarg);   set_mux      = true; break; }
      case 'p': { io_pps         = atoi(optarg);   set_pps      = true; break; }
      case 'd': { io_drive       = atoi(optarg);   set_drive    = true; break; }
      case 'k': { stime          = atoi(optarg);   set_stime    = true; break; }
      case 's': { ios_snoop      = true; break; }
      case 'w': { ios_wipe       = true; break; }
      case 'y': { ios_i_to_e     = true; break; }
      case 'b': { ios_setup_only = true;
                  if (argv[optind-1] != NULL) { prefix = strtoull(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing prefix!" << std::endl; return (__IO_RETURN_FAILURE); }
                  break; }
      case 'r': { ioc_dis_ios    = true;
                  ios_setup_only = true;
                  break; }
      case 'c': { ioc_create     = true;
                  if (argv[optind-1] != NULL) { eventID = strtoull(argv[optind-1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing event id!" << std::endl; return (__IO_RETURN_FAILURE); }
                  if (argv[optind+0] != NULL) { eventMask = strtoull(argv[optind+0], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing event mask!" << std::endl; return (__IO_RETURN_FAILURE); }
                  if (argv[optind+1] != NULL) { offset = strtoull(argv[optind+1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing offset!" << std::endl; return (__IO_RETURN_FAILURE); }
                  if (argv[optind+2] != NULL) { flags = strtoull(argv[optind+2], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing flags!" << std::endl; return (__IO_RETURN_FAILURE); }
                  if (argv[optind+3] != NULL) { level = strtoull(argv[optind+3], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing level!" << std::endl; return (__IO_RETURN_FAILURE); }
                  break; }
      case 'g': { negative       = true; break; }
      case 'u': { ioc_disown     = true; break; }
      case 'x': { ioc_destroy    = true; break; }
      case 'f': { ioc_flip       = true; break; }
      case 'z': { translate_mask = true; break; }
      case 'l': { ioc_list       = true; break; }
      case 'i': { show_table     = true; break; }
      case 'v': { verbose_mode   = true; break; }
      case 'h': { show_help      = true; break; }
      case ':': { std::cout << "Option -" << (char)optopt << " requires argument(s)!" << std::endl; show_help = true; break; }
      default:  { std::cout << "Unknown option: " << (char)optopt << std::endl; show_help = true; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }

  /* Help wanted? */
  if (show_help) { io_help(); return (__IO_RETURN_SUCCESS); }

  /* Plausibility check for arguments */
  if ((ioc_create || ioc_disown) && ioc_destroy) { std::cerr << "Incorrect arguments!" << std::endl; return (__IO_RETURN_FAILURE); }
  else                                           { ioc_valid = true; }

  /* Plausibility check for event input options */
  if      (ios_snoop && ios_wipe)       { std::cerr << "Incorrect arguments (disable input events or snoop)!"  << std::endl; return (__IO_RETURN_FAILURE);}
  else if (ios_snoop && ios_setup_only) { std::cerr << "Incorrect arguments (enable input events or snoop)!"   << std::endl; return (__IO_RETURN_FAILURE);}
  else if (ios_wipe  && ios_setup_only) { std::cerr << "Incorrect arguments (disable or enable input events)!" << std::endl; return (__IO_RETURN_FAILURE); }

  /* Get basic arguments, we need at least the device name */
  if (optind + 1 == argc)
  {
    deviceName = argv[optind]; /* Get the device name */
    if ((io_oe || io_term || io_spec_out || io_spec_in || io_gate_out || io_gate_in || io_mux || io_pps || io_drive || show_help || show_table ||
         ios_snoop || ioc_create || ioc_disown || ioc_destroy || ioc_flip || ioc_list || ioNameGiven || ioc_valid) == false)
    {
      std::cerr << "Incorrect non-optional arguments (expecting at least the device name and one additional argument)(arg)!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }
  }
  else if (ioc_valid)
  {
    deviceName = argv[optind]; /* Get the device name */
  }
  else
  {
    std::cerr << "Incorrect non-optional arguments (expecting at least the device name and one additional argument)(dev)!" << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Confirm device exists */
  if (deviceName == NULL)
  {
    std::cerr << "Missing device name!" << std::endl;
    return (__IO_RETURN_FAILURE);
  }
//  Gio::init();
  map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
  if (devices.find(deviceName) == devices.end())
  {
    std::cerr << "Device " << deviceName << " does not exist!" << std::endl;
    return (__IO_RETURN_FAILURE);
  }
  if (verbose_mode) { std::cout << "Device " << deviceName << " found!" << std::endl; }

  /* Plausibility check */
  if (io_oe > 1       || io_oe < 0)       { std::cout << "Error: Output enable setting is invalid!"             << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_term > 1     || io_term < 0)     { std::cout << "Error: Input termination setting is invalid"          << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_spec_out > 1 || io_spec_out < 0) { std::cout << "Error: Special (output) function setting is invalid!" << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_spec_in > 1  || io_spec_in < 0)  { std::cout << "Error: Special (input) function setting is invalid!"  << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_gate_out > 1 || io_gate_out < 0) { std::cout << "Error: Gate (output) setting is invalid!"             << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_gate_in > 1  || io_gate_in < 0)  { std::cout << "Error: Gate (input) setting is invalid!"              << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_mux > 1      || io_mux < 0)      { std::cout << "Error: BuTiS t0 gate/mux setting is invalid!"         << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_pps > 1      || io_pps < 0)      { std::cout << "Error: WR PPS gate/mux setting is invalid!"           << std::endl; return (__IO_RETURN_FAILURE); }
  if (io_drive > 1    || io_drive < 0)    { std::cout << "Error: Output value is not valid!"                    << std::endl; return (__IO_RETURN_FAILURE); }
  if (set_stime)
  {
    if (stime % 8 != 0)                     { std::cout << "Error: StableTime must be a multiple of 8 ns"         << std::endl; return (__IO_RETURN_FAILURE); }
    if (stime < 16)                         { std::cout << "Error: StableTime must be at least 16 ns"             << std::endl; return (__IO_RETURN_FAILURE); }
  }

  /* Check if given IO name exists */
  if (ioNameGiven)
  {
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > outs;
    std::map< std::string, std::string > ins;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();

    /* Check all inputs and outputs */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (it->first == ioName)          { ioNameExists = true; }
      if (verbose_mode && ioNameExists) { std::cout << "IO " << ioName << " found (output)!" << std::endl; break; }
    }
    for (std::map<std::string,std::string>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      if (it->first == ioName)          { ioNameExists = true; }
      if (verbose_mode && ioNameExists) { std::cout << "IO " << ioName << " found (input)!" << std::endl; break; }
    }

    /* Inform the user if the given IO does not exist */
    if (!ioNameExists)
    {
      std::cerr << "IO " << ioName << " does not exist!" << std::endl;
      return (__IO_RETURN_FAILURE);
    }
  }

  /* Check if help is needed, otherwise evaluate given arguments */
  if (show_help) { io_help(); }
  else
  {
    /* Proceed with program */
    if      (bg_fw_id)       { return_code = bg_read_fw_id(); }
    else if (bg_instr)       { return_code = bg_instruct(bg_instr_args); }
    else if (bg_cfg_io)      { return_code = bg_config_io(bg_t_high, bg_t_p, bg_t_b, bg_b_delay, bg_b_flag, verbose_mode); }
    else if (bg_clr_all)     { return_code = bg_clear_all(verbose_mode); }
    else if (bg_ls_bursts)   { return_code = bg_list_bursts(bg_id, verbose_mode); }
    else if (bg_rm_burst)    { return_code = bg_remove_burst(bg_id, verbose_mode); }
    else if (bg_disen_burst) { return_code = bg_disenable_burst(bg_id, bg_disen, verbose_mode); }
    else if (bg_mk_burst)    { return_code = bg_create_burst(burstId, bg_e_id, bg_e_mask, verbose_mode); }
    else if (show_table)     { return_code = io_print_table(verbose_mode); }
    else if (ios_wipe)       { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_snoop)      { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_setup_only) { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_i_to_e)     { return_code = io_list_i_to_e(); }
    else if (ioc_create)     { return_code = io_create(ioc_disown, eventID, eventMask, offset, flags, level, negative, translate_mask); }
    else if (ioc_destroy)    { return_code = io_destroy(verbose_mode); }
    else if (ioc_flip)       { return_code = io_flip(verbose_mode); }
    else if (ioc_list)       { return_code = io_list(); }
    else                     { return_code = io_setup(io_oe, io_term, io_spec_out, io_spec_in, io_gate_out, io_gate_in, io_mux, io_pps, io_drive, stime, set_oe, set_term, set_spec_out, set_spec_in, set_gate_out, set_gate_in, set_mux, set_pps, set_drive, set_stime, verbose_mode); }
  }

  /* Done */
  return (return_code);
}