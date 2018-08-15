// @file saft-ctl.cpp
// @brief Command-line interface for saftlib. This tool focuses on the software part.
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// Have a chat with saftlib
//
//*****************************************************************************
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
//

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <iomanip>
#include <giomm.h>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/iOwned.h"
#include "src/CommonFunctions.h"

#include <pthread.h>

static guint32 pmode = PMODE_NONE;    // how data are printed (hex, dec, verbosity)

// this will be called, in case we are snooping for events
static void on_action(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  std::cout << std::this_thread::get_id() << ": tDeadline: " << tr_formatDate(deadline, pmode);
  std::cout << tr_formatActionEvent(id, pmode);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode);
  std::cout << std::endl;
} // on_action


// runs in separate thread
void* snoop(void *data) 
{
  Glib::RefPtr<Glib::MainContext> context = Glib::MainContext::create();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create(context);
  saftbus::ProxyConnection::set_default_context(context);
  Glib::ustring deviceName = "tr0";
  std::cerr << "snoop tread : " << pthread_self() << std::endl;
  Glib::RefPtr<saftlib::SAFTd_Proxy> saftd = saftlib::SAFTd_Proxy::create();
  std::cerr << "snoop tread : got a TimingReceiver_Proxy :)" << std::endl;    

  std::map<Glib::ustring, Glib::ustring> devices = saftd->getDevices();
  std::cerr << "snoop tread : got the devices list" << std::endl;
  for (auto it = devices.begin(); it != devices.end(); ++it) {
   std::cerr << "snoop tread : " << it->first << ":" << it->second << std::endl;
  }
  Glib::RefPtr<saftlib::TimingReceiver_Proxy> receiver = saftlib::TimingReceiver_Proxy::create(devices[deviceName]);
  Glib::RefPtr<saftlib::SoftwareActionSink_Proxy> sink = saftlib::SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
  Glib::RefPtr<saftlib::SoftwareCondition_Proxy> condition = saftlib::SoftwareCondition_Proxy::create(sink->NewCondition(false, 0, 0, 0));
  // Accept all errors
  condition->setAcceptLate(true);
  condition->setAcceptEarly(true);
  condition->setAcceptConflict(true);
  condition->setAcceptDelayed(true);
  condition->Action.connect(sigc::ptr_fun(&on_action));
  condition->setActive(true);

  loop->run();
  return nullptr;
}

int main(int argc, char** argv)
{
 
  try {
    // initialize required stuff
    Gio::init();
    //Glib::thread_init();
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    //Glib::ustring deviceName = "tr0";
    

    //snoop(nullptr);

    // snoop
    if (true) {
      pthread_t thread1;
      int tid1 = pthread_create(&thread1, nullptr, snoop, nullptr);
    } // eventSnoop

    //sleep(2);

    if (true) {
      pthread_t thread1;
      int tid1 = pthread_create(&thread1, nullptr, snoop, nullptr);
    } // eventSnoop

    //sleep(2);

    std::cerr << " ******* main tread " << std::endl;

    // // before starting the second Glib::MainLoop, the Saftbus call_sync has to be 
    // //  finished, otherwise there will be chaos!
    // //sleep(1);

    // before any proxy is used, the MainContext in which it should run has to be set
    saftbus::Proxy::set_default_context(Glib::MainContext::get_default());
    //get a specific device
    std::map<Glib::ustring, Glib::ustring> devices = saftlib::SAFTd_Proxy::create()->getDevices();
    for (auto it = devices.begin(); it != devices.end(); ++it) {
      std::cerr << " ******* main thread: " << it->first << ":" << it->second << std::endl;
    }


  Glib::RefPtr<saftlib::TimingReceiver_Proxy> receiver = saftlib::TimingReceiver_Proxy::create(devices["tr0"]);
  Glib::RefPtr<saftlib::SoftwareActionSink_Proxy> sink = saftlib::SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
  Glib::RefPtr<saftlib::SoftwareCondition_Proxy> condition = saftlib::SoftwareCondition_Proxy::create(sink->NewCondition(false, 0, 0, 0));
  // Accept all errors
  condition->setAcceptLate(true);
  condition->setAcceptEarly(true);
  condition->setAcceptConflict(true);
  condition->setAcceptDelayed(true);
  condition->Action.connect(sigc::ptr_fun(&on_action));
  condition->setActive(true);


    // // std::cerr << "starting main thread main loop" << std::endl;
    loop->run();


  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}



