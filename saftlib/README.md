# Saftlib Simplified API for Timing

Saflib is a library and a set of tools to use and control the FAIR Timing Receiver hardware.
Saftlib provides
  - A C++ class library to access FAIR Timing Receiver Hardware. It can be used to configure the ECA channels, create conditions and receive software Interrupts from timing events. 
  - A library that can be directly by stand alone programs.
  - A library that can be used to share hardware resources by access through [saftbus](../saftbus/README.md) via IPC (interprocess communication).
  - A set of command line tools to control and interact with attached hardware devices.

## User guide

### A simple example

A simple program that can be used to monitor timing events.
The driver classes can either be used through Proxy objects. In this case many processes can share the hardware resources at the same time.

Alternatively, the driver classes can also be used directly for lower latency (standalone, without running saftbusd). In this case, no saftbusd with the saftd-service can run at the same time. But the program can be written in a way to provide the same functionality, if needed. 

The program configures the ECA to effectively call the function "on_action" whenever an event matches the specified event id.

The configuration of the ECA always follows these steps:
  - Create a Proxy of the desired ECA output channel (aka action sink). In this case it is the SoftwareActionSink_Proxy. (Best Practice: Proxy objects should be stored by program their service is frequently used, e.g. when new ECA conditions are frequently defined. This avoids unnecessary overhead for creating and destroying the Proxy every time a condition is configured.)
  - Create a new Condition on the SoftwareActionSink, the return value of the function is the object path of the newly created condition.
  - Create a SoftwareCondition_Proxy object for the newly created condition.
  - Connect the callback function to the "SigAction" signal of the SoftwareCondition_Proxy.
  - Enter a loop and wait for signals.

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





