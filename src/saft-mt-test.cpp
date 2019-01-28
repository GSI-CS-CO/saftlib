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



int rt_event_pipe[2]; // read[0] write[1]
Glib::RefPtr<saftlib::TimingReceiver_Proxy> receiver;
Glib::RefPtr<saftlib::SoftwareActionSink_Proxy> sink;
Glib::RefPtr<saftlib::SoftwareCondition_Proxy> condition;

// this will be called, in case we are snooping for events
static void on_action(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  std::cout << "tDeadline: " << tr_formatDate(deadline, pmode);
  std::cout << tr_formatActionEvent(id, pmode);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode);
  std::cout << std::endl;
  char ch = 'e';
  write(rt_event_pipe[1], &ch, 1); // start th RT-Action
} // on_action

void *mainloop(void *data)
{
  // snoop for events
  while(true) {
    saftlib::wait_for_signal();
  }
  return nullptr;
}



bool rtAction(Glib::IOCondition condition)
{
  char ch;
  read(rt_event_pipe[0], &ch, 1);
  std::cerr << "rtAction called: ch = " << ch << std::endl;
  return true;
}

void *rtActionLoop(void *data)
{
  Glib::RefPtr<Glib::MainContext> context = Glib::MainContext::create();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create(context);

  context->signal_io().connect(sigc::ptr_fun(rtAction), rt_event_pipe[0], Glib::IO_IN | Glib::IO_HUP, Glib::PRIORITY_HIGH);
  loop->run();
  return nullptr;
}

int main(int argc, char** argv)
{
  Glib::init();

  if (pipe(rt_event_pipe) != 0) {
    std::cerr << " couldn't open pipe " << std::endl;
    return 1;
  }

  Glib::RefPtr<saftlib::SAFTd_Proxy> saftd = saftlib::SAFTd_Proxy::create();
  std::map<std::string, std::string> devices = saftd->getDevices();

  receiver  = saftlib::TimingReceiver_Proxy::create(devices["tr0"]);
  sink      = saftlib::SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
  condition = saftlib::SoftwareCondition_Proxy::create(sink->NewCondition(false, 0, 0, 0));
  condition->setAcceptLate(true);
  condition->setAcceptEarly(true);
  condition->setAcceptConflict(true);
  condition->setAcceptDelayed(true);
  condition->Action.connect(sigc::ptr_fun(&on_action));
  condition->setActive(true);

  pthread_t thread1, thread2;

  pthread_create(&thread1, nullptr, mainloop,     nullptr);
  pthread_create(&thread2, nullptr, rtActionLoop, nullptr);


  pthread_join(thread1, nullptr);
  pthread_join(thread2, nullptr);

  return 0;
}
