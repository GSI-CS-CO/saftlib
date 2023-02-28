# Saftlib Simplified API for Timing

## Overview

Saflib is a library and a set of tools to use and control the FAIR Timing Receiver hardware.
Saftlib provides
  - A C++ class library to access FAIR Timing Receiver Hardware. It can be used to configure the ECA channels, create conditions and receive software Interrupts from timing events. 
  - A library of proxy classes that can be used to share hardware resources by access through [saftbus](../saftbus/README.md) via IPC (interprocess communication).
  - A library that can be directly by stand alone programs.
  - A set of command line tools to control and interact with attached hardware devices.

## Running saftd-service.so

The most common use case for saftlib, is to be used over saftbus. A running saftbusd with the libsaftd-servcie.so plugin is needed and a TimingReceiver_Service has to be attached to SAFTd. This can be achieve by launching saftbusd like this (assuming that a FAIR Timing Receiver is attached to the system under /dev/wmb0):

    saftbusd libsaftd-service.so tr0:dev/wbm0

Alternatively, saftbusd can be launched without any plugins and libsaftd-service.so can be loaded later using saftbus-ctl

    saftbus-ctl -l libsaftd-service.so tr0:dev/wbm0

In order to see, which plugins are loaded, which services are available on saftbusd, and which processes are connected to saftbus,  saftbus-ctl can be used:
```bash
$ saftbus-ctl -s
objects:
  object-path                        ID [owner] sig-fd/use-count interface-names
  /saftbus                           1 [-1]  11/1 Container 
  /de/gsi/saftlib                    2 [-1]d SAFTd 
  /de/gsi/saftlib/tr0/acwbm          3 [-1]  WbmActionSink ActionSink Owned 
  /de/gsi/saftlib/tr0/embedded_cpu   4 [-1]  EmbeddedCPUActionSink ActionSink Owned 
  /de/gsi/saftlib/tr0/outputs/LED1   5 [-1]  Output ActionSink Owned 
  [...]
  /de/gsi/saftlib/tr0/inputs/LVDSi2  33 [-1]  Input EventSource Owned 
  /de/gsi/saftlib/tr0/inputs/LVDSi3  34 [-1]  Input EventSource Owned 
  /de/gsi/saftlib/tr0                35 [-1]d TimingReceiver BuildIdRom ECA ECA_Event ECA_TLU LM32Cluster Mailbox OpenDevice Reset TempSensor Watchdog WhiteRabbit 

active plugins: 
  libsaftd-service.so

connected client processes:
  10 (pid=432932)
```


## Developing with saftlib

### Best practices 
In order to get the best performance out of saftlib, users should follow some basic rules:
  - If a Proxy objects is used frequently int the program it should be stored instead of recreated if it is needed. For example, when new ECA conditions are frequently defined, the corresponding ActionSink Proxy should be stored instad of calling ActionSink::create() whenever an ActionSink_Proxy is needed. Reason: Creating a Proxy object has some overhead, it loads the program as well as the saftbusd server. 

### Limitations
There are some limitations that users should be aware of.
  - It is not possible to create or destroy Proxy objects on a saftbus::SignalGroup inside a Signal callback function from the same saftbus::SignalGroup.


### Shared access or exclusive access  
There are two fundamentally different ways of writing code that uses saftlib:
  - The classes from saftlib can be used remotely through Proxy objects, this is shown in the first code example. In this case many processes can share the hardware resources at the same time. A saftbusd server has to be available with the saftd-service plugin loaded and a TimingReceiver hardware attached. This is probably the most common use case.
  - Alternatively, the driver classes can also be used directly for lower latency (standalone, without running saftbusd). In this case the program connects directly to the hardware and no saftbusd server can run connect to the same hardware at the same time. But the program can be written in a way to provide the same functionality as saftbusd. This option might be useful when extra low latency in the hardware communication is needed.

Both ways are shown in the following example, a simple event snoop tool. It is is first written using the Proxy objects, then it is shown how to get the same functionality using the driver classes directly.

### A simple event snoop tool

A simple program that can be used to monitor timing events.
The program configures the ECA to effectively call the function "on_action" whenever an event matches the specified event id.

The configuration of the ECA always follows these steps:
  - Create a Proxy of the desired ECA output channel (aka action sink). In this case it is the SoftwareActionSink_Proxy. 
  - Create a new Condition on the SoftwareActionSink, the return value of the function is the object path of the newly created condition.
  - Create a SoftwareCondition_Proxy object for the newly created condition.
  - Connect the callback function to the "SigAction" signal of the SoftwareCondition_Proxy.
  - Enter a loop and wait for signals.

The relevant classes for this example are (you may need to run doxygen for the links to work)
  - [saftlib::SAFTd](html/classsaftlib_1_1SAFTd.html)
  - [saftlib::TimingReceiver](html/classsaftlib_1_1TimingReceiver.html)
  - [saftlib::softwareActionSink](html/classsaftlib_1_1SoftwareActionSink.html)
  - [saftlib::softwareCondition](html/classsaftlib_1_1SoftwareCondition.html)

#### Using saflib Proxy objects with saftbusd
Create a file "saft-snoop-saftbus.cpp":
```C++
#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SoftwareActionSink_Proxy.hpp>
#include <SoftwareCondition_Proxy.hpp>

#include <saftbus/client.hpp>

#include <memory>
#include <iostream>

void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
  std::cout << "event " << event << " " 
            << "param " << param << " " 
            << "deadline " << deadline.getTAI() << " "
            << "executed " << executed.getTAI() << " " 
            << "flags " << flags << " "
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <saftlib-device> <id> <mask> <param>" << std::endl;
    return 0;
  }

  uint64_t snoop_args[3]; 
  uint64_t &id    = snoop_args[0];
  uint64_t &mask  = snoop_args[1];
  uint64_t &param = snoop_args[2];
  for (int i = 2; i < 5; ++i) {
    std::istringstream argin(argv[i]);
    argin >> snoop_args[i-2];
    if (!argin) {
      std::cerr << "cannot read snoop argument from " << argv[i] << std::endl;
      return 1;
    }
  }

  auto saftd = saftlib::SAFTd_Proxy::create();
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[argv[1]]);
  auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
  auto softwareActionSink          = saftlib::SoftwareActionSink_Proxy::create(softwareActionSink_obj_path);
  auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);
  auto sw_condition                = saftlib::SoftwareCondition_Proxy::create(condition_obj_path);

  sw_condition->setAcceptEarly(true);
  sw_condition->setAcceptLate(true);
  sw_condition->setAcceptDelayed(true);
  sw_condition->setAcceptConflict(true);

  sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));

  for (;;) {
    //saftbus::Loop::get_default().iteration(true); 
    saftbus::SignalGroup::get_global().wait_for_signal();
  }

  return 0;
}
```
Compile with: 

    g++ -o saft-snoop-saftbus saft-snoop-saftbus.cpp `pkg-config saftlib --cflags --libs`

#### Using saftlib Driver classes directly (standalone mode)
Create a file "saft-snoop-standalone.cpp"
```C++
#include <SAFTd.hpp>
#include <SAFTd_Service.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>

#include <saftbus/client.hpp>
#include <saftbus/server.hpp>

#include <memory>
#include <iostream>

void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
  std::cout << "event " << event << " " 
            << "param " << param << " " 
            << "deadline " << deadline.getTAI() << " "
            << "executed " << executed.getTAI() << " " 
            << "flags " << flags << " "
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <etherbone-path> <id> <mask> <param>" << std::endl;
    return 0;
  }

  uint64_t snoop_args[3]; 
  uint64_t &id    = snoop_args[0];
  uint64_t &mask  = snoop_args[1];
  uint64_t &param = snoop_args[2];
  for (int i = 2; i < 5; ++i) {
    std::istringstream argin(argv[i]);
    argin >> snoop_args[i-2];
    if (!argin) {
      std::cerr << "cannot read snoop argument from " << argv[i] << std::endl;
      return 1;
    }
  }

  // the next lines makes a fully functional saftd out of the program 
  saftbus::ServerConnection server_connection;
  saftbus::Container *container = server_connection.get_container();
  saftlib::SAFTd saftd(container); 
  std::unique_ptr<saftlib::SAFTd_Service> saftd_service(new saftlib::SAFTd_Service(&saftd));
  container->create_object("/de/gsi/saftlib", std::move(saftd_service));
  // end of saftd part

  auto tr_obj_path                 = saftd.AttachDevice("tr0", argv[1], 100);                  
  saftlib::TimingReceiver* tr      = saftd.getTimingReceiver(tr_obj_path);                     
  auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");                            
  auto softwareActionSink          = tr->getSoftwareActionSink(softwareActionSink_obj_path);   
  auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);  
  auto sw_condition                = softwareActionSink->getCondition(condition_obj_path);     

  sw_condition->setAcceptEarly(true);
  sw_condition->setAcceptLate(true);
  sw_condition->setAcceptDelayed(true);
  sw_condition->setAcceptConflict(true);

  sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));

  for (;;) {
    saftbus::Loop::get_default().iteration(true); 
  }

  return 0;
}
```
Compile with:

    g++ -o saft-snoop-standalone saft-snoop-standalone.cpp `pkg-config saftlib --cflags --libs`





