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
/* IO control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#define ECA_EVENT_ID_LATCH     UINT64_C(0xfffe000000000000) /* FID=MAX & GRPID=MAX-1 */
#define ECA_EVENT_MASK_LATCH   UINT64_C(0xfffe000000000000)
#define IO_CONDITION_OFFSET    5000

/* Includes */
/* ==================================================================================================== */
#include <iostream>
#include <iomanip>
#include <giomm.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <string>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/Output.h"
#include "interfaces/Input.h"
#include "interfaces/OutputCondition.h"

#include "src/CommonFunctions.h"

#include "drivers/eca_flags.h"
#include "drivers/io_control_regs.h"

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
std::map<Glib::ustring,guint64>     map_PrefixName; /* Translation table IO name <> prefix */

/* Prototypes */
/* ==================================================================================================== */
static void io_help        (void);
static int  io_setup       (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_mux, int io_pps, int io_drive, int stime,
                            bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_mux, bool set_pps, bool set_drive, bool set_stime,
                            bool verbose_mode);
static int  io_create      (bool disown, guint64 eventID, guint64 eventMask, gint64 offset, guint64 flags, gint64 level, bool offset_negative, bool translate_mask);
static int  io_destroy     (bool verbose_mode);
static int  io_flip        (bool verbose_mode);
static int  io_list        (void);
static int  io_list_i_to_e (void);
static int  io_print_table (bool verbose_mode);
static void io_catch_input (guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags);
static int  io_snoop       (bool mode, bool setup_only, bool disable_source, guint64 prefix_custom);

/* Function io_create() */
/* ==================================================================================================== */
static int io_create (bool disown, guint64 eventID, guint64 eventMask, gint64 offset, guint64 flags, gint64 level, bool offset_negative, bool translate_mask)
{
  /* Helpers */
  bool   io_found          = false;
  bool   io_edge           = false;
  bool   io_AcceptConflict = false;
  bool   io_AcceptDelayed  = false;
  bool   io_AcceptEarly    = false;
  bool   io_AcceptLate     = false;
  gint64 io_offset         = offset;

  /* Get level/edge */
  if (level > 0) { io_edge = true; }
  else           { io_edge = false; }

  /* Perform selected action(s) */
  try
  {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    Glib::ustring io_path;
    outs = receiver->getOutputs();
    Glib::RefPtr<Output_Proxy> output_proxy;

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
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
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
    Glib::RefPtr<OutputCondition_Proxy> condition;
    if (translate_mask) { condition = OutputCondition_Proxy::create(output_proxy->NewCondition(true, eventID, tr_mask(eventMask), io_offset, io_edge)); }
    else                { condition = OutputCondition_Proxy::create(output_proxy->NewCondition(true, eventID, eventMask, io_offset, io_edge)); }
    condition->setAcceptConflict(io_AcceptConflict);
    condition->setAcceptDelayed(io_AcceptDelayed);
    condition->setAcceptEarly(io_AcceptEarly);
    condition->setAcceptLate(io_AcceptLate);

    /* Disown and quit or keep waiting */
    if (disown) { condition->Disown(); }
    else        { std::cout << "Condition created..." << std::endl; loop->run(); }
  }
  catch (const Glib::Error& error)
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
  Glib::ustring c_name = "Unknown";
  std::vector <Glib::RefPtr<OutputCondition_Proxy> > prox;

  /* Perform selected action(s) */
  try
  {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    Glib::ustring io_path;
    outs = receiver->getOutputs();
    Glib::RefPtr<Output_Proxy> output_proxy;

    /* Check if IO exists output */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      output_proxy = Output_Proxy::create(it->second);
      std::vector< Glib::ustring > all_conditions = output_proxy->getAllConditions();
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          Glib::RefPtr<OutputCondition_Proxy> destroy_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
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
  catch (const Glib::Error& error)
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
  Glib::ustring c_name = "Unknown";

  /* Perform selected action(s) */
  try
  {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    Glib::ustring io_path;
    outs = receiver->getOutputs();
    Glib::RefPtr<Output_Proxy> output_proxy;
    std::vector< Glib::ustring > conditions_to_destroy;

    /* Check if IO exists output */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      output_proxy = Output_Proxy::create(it->second);
      std::vector< Glib::ustring > all_conditions = output_proxy->getAllConditions();
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          Glib::RefPtr<OutputCondition_Proxy> flip_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
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
  catch (const Glib::Error& error)
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
  Glib::ustring c_name  = "Unknown";

  /* Perform selected action(s) */
  try
  {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    Glib::ustring io_path;
    outs = receiver->getOutputs();
    Glib::RefPtr<Output_Proxy> output_proxy;

    /* Check if IO exists output */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
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
      std::vector< Glib::ustring > all_conditions = output_proxy->getAllConditions();

      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
        {
          /* Helper for flag information field */
          guint32  flags = 0x0;

          /* Get output conditions */
          Glib::RefPtr<OutputCondition_Proxy> info_condition = OutputCondition_Proxy::create(all_conditions[condition_it]);
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
  catch (const Glib::Error& error)
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
  std::cout << "IO-CTL for SAFTlib " << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -n <name>:                                     Specify IO name or leave blank (to see all IOs/conditions)" << std::endl;
  std::cout << std::endl;
  std::cout << "  -o 0/1:                                        Toggle output enable" << std::endl;
  std::cout << "  -t 0/1:                                        Toggle termination/resistor" << std::endl;
  std::cout << "  -q 0/1:                                        Toggle special (output) functionality" << std::endl;
  std::cout << "  -e 0/1:                                        Toggle special (input) functionality" << std::endl;
  std::cout << "  -m 0/1:                                        Toggle BuTiS t0 + TS gate/mux" << std::endl;
  std::cout << "  -p 0/1:                                        Toggle WR PPS gate/mux" << std::endl;
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
  std::cout << std::endl;
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
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > inputs = receiver->getInputs();

    /* Check inputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=inputs.begin(); it!=inputs.end(); ++it)
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
        guint64 prefix = ECA_EVENT_ID_LATCH + (map_PrefixName.size()*2);
        map_PrefixName[ioName] = prefix;
        Glib::RefPtr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);

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
  catch (const Glib::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_catch_input() */
/* ==================================================================================================== */
static void io_catch_input(guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  /* Helpers */
  guint64 time = deadline - IO_CONDITION_OFFSET;
  Glib::ustring catched_io = "Unknown";

  /* !!! evaluate prefix<>name map */
  for (std::map<Glib::ustring,guint64>::iterator it=map_PrefixName.begin(); it!=map_PrefixName.end(); ++it) { if (event == it->second) { catched_io = it->first; } } /* Rising */
  for (std::map<Glib::ustring,guint64>::iterator it=map_PrefixName.begin(); it!=map_PrefixName.end(); ++it) { if (event-1 == it->second) { catched_io = it->first; } } /* Falling */

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
static int io_snoop(bool mode, bool setup_only, bool disable_source, guint64 prefix_custom)
{
  /* Helpers (connect proxies in a vector) */
  std::vector <Glib::RefPtr<SoftwareCondition_Proxy> > proxies;
  std::vector <Glib::RefPtr<SoftwareActionSink_Proxy> > sinks;

  /* Get inputs and snoop */
  try
  {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > inputs = receiver->getInputs();

    /* Check inputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        /* Set name */
        ioName =  it->first.c_str();
        guint64 prefix = ECA_EVENT_ID_LATCH + (map_PrefixName.size()*2);
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
          Glib::RefPtr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);
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
          Glib::RefPtr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);
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
        loop->run();
      }
    }
    else
    {
      std::cout << "This Timing Receiver has no inputs!" << std::endl;
    }

  }
  catch (const Glib::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }

  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_setup() */
/* ==================================================================================================== */
static int io_setup (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_mux, int io_pps, int io_drive, int stime,
                    bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_mux, bool set_pps, bool set_drive, bool set_stime,
                    bool verbose_mode)
{
  unsigned io_type  = 0;     /* Out, Inout or In? */
  bool     io_found = false; /* IO exists? */
  bool     io_set   = false; /* IO set or get configuration */

  /* Check if there is at least one parameter to set */
  io_set = set_oe | set_term | set_spec_out | set_spec_in | set_mux | set_pps | set_drive | set_stime;

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
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();

  /* Perform selected action(s) */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);

    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    Glib::ustring io_path;
    Glib::ustring io_partner;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    Glib::RefPtr<Output_Proxy> output_proxy;
    Glib::RefPtr<Input_Proxy> input_proxy;

    /* Check if IO exists as input */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
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
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
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

        /* Display BuTiS multiplexer state */
        if (output_proxy->getBuTiSMultiplexer())    { std::cout << "  BuTiS t0 + TS:    On" << std::endl; }
        else                                        { std::cout << "  BuTiS t0 + TS:    Off" << std::endl; }

        /* Display WR PPS multiplexer state */
        if (output_proxy->getPPSMultiplexer())      { std::cout << "  WR PPS:           On" << std::endl; }
        else                                        { std::cout << "  WR PPS:           Off" << std::endl; }
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

        /* Display stable time */
        std::cout << "  StableTime:       " << input_proxy->getStableTime() << " ns" << std::endl;

      }
    }
  }
  catch (const Glib::Error& error)
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
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();

  /* Try to get the table */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();

    /* Show verbose output */
    if (verbose_mode)
    {
      std::cout << "Discovered Output(s): " << std::endl;
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
      std::cout << "Discovered Inputs(s) " << std::endl;
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
    }

    /* Print table header */
    std::cout << "Name           Direction  OutputEnable  InputTermination  SpecialOut  SpecialIn  Resolution  Logic Level" << std::endl;
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

    /* Print Outputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        Glib::RefPtr<Output_Proxy> output_proxy;
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
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        Glib::RefPtr<Input_Proxy> input_proxy;
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
  catch (const Glib::Error& error)
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
  int  io_mux         = 0;     /* Gate (BuTiS) */
  int  io_pps         = 0;     /* Gate (PPS) */
  int  io_drive       = 0;     /* Drive IO value */
  int  ioc_flip       = 0;     /* Flip active bit for all conditions */
  int  stime          = 0;     /* Stable time to set */
  int  return_code    = 0;     /* Return code */
  guint64 eventID     = 0x0;   /* Event ID (new condition) */
  guint64 eventMask   = 0x0;   /* Event mask (new condition) */
  gint64  offset      = 0x0;   /* Event offset (new condition) */
  guint64 flags       = 0x0;   /* Accept flags (new condition) */
  gint64  level       = 0x0;   /* Rising or falling edge (new condition) */
  guint64 prefix      = 0x0;   /* IO input prefix */
  bool translate_mask = false; /* Translate mask? */
  bool negative       = false; /* Offset negative? */
  bool ioc_valid      = false; /* Create arguments valid? */
  bool set_oe         = false; /* Set? */
  bool set_term       = false; /* Set? */
  bool set_spec_in    = false; /* Set? */
  bool set_spec_out   = false; /* Set? */
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

  /* Get the application name */
  program = argv[0];
  deviceName = "NoDeviceNameGiven";

  /* Parse for options */
  while ((opt = getopt(argc, argv, "n:o:t:q:e:m:p:d:k:swyb:rc:guxfzlivh")) != -1)
  {
    switch (opt)
    {
      case 'n': { ioName         = argv[optind-1]; ioNameGiven  = true; break; }
      case 'o': { io_oe          = atoi(optarg);   set_oe       = true; break; }
      case 't': { io_term        = atoi(optarg);   set_term     = true; break; }
      case 'q': { io_spec_out    = atoi(optarg);   set_spec_out = true; break; }
      case 'e': { io_spec_in     = atoi(optarg);   set_spec_in  = true; break; }
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
      default:  { std::cout << "Unknown argument..." << std::endl; show_help = true; break; }
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
    if ((io_oe || io_term || io_spec_out || io_spec_in || io_mux || io_pps || io_drive || show_help || show_table ||
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
  Gio::init();
  map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
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
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();

    /* Check all inputs and outputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (it->first == ioName)          { ioNameExists = true; }
      if (verbose_mode && ioNameExists) { std::cout << "IO " << ioName << " found (output)!" << std::endl; break; }
    }
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
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
    if      (show_table)     { return_code = io_print_table(verbose_mode); }
    else if (ios_wipe)       { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_snoop)      { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_setup_only) { return_code = io_snoop(ios_wipe, ios_setup_only, ioc_dis_ios, prefix); }
    else if (ios_i_to_e)     { return_code = io_list_i_to_e(); }
    else if (ioc_create)     { return_code = io_create(ioc_disown, eventID, eventMask, offset, flags, level, negative, translate_mask); }
    else if (ioc_destroy)    { return_code = io_destroy(verbose_mode); }
    else if (ioc_flip)       { return_code = io_flip(verbose_mode); }
    else if (ioc_list)       { return_code = io_list(); }
    else                     { return_code = io_setup(io_oe, io_term, io_spec_out, io_spec_in, io_mux, io_pps, io_drive, stime, set_oe, set_term, set_spec_out, set_spec_in, set_mux, set_pps, set_drive, set_stime, verbose_mode); }
  }

  /* Done */
  return (return_code);
}
