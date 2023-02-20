# Saftlib Simplified API for Timing

Saflib is a library and a set of tools to use and control the FAIR Timing Receiver hardware.
Saftlib provides the following
  - A C++ class library to access Fair Timing Receiver Hardware. It can be used to configure the ECA channels, create conditions and receive software Interrupts from timing events. 
  - This library can be used in stand-alone mode, or it can be dynamically loaded as a plugin into a running [saftbus](../saftbus/README.md) daemon.
  - Various command line tools to control and interact with attached hardware devices.

## User guide

### A simple example

A simple tool that can be used to monitor timing events.
The driver classes can either be used throug Proxy objects. In this case many processes can share the hardware resources at the same time.

Or the driver classes can also be used directly for lower latency (standalone, without running saftbusd). In this case, saftbusd funkctionality can be baked into the program to still allow sharing of hardware resources. 

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
#### TimingReceiver
Each SDB device on the hardware is represented by one C++ class. 
The TimingReceiver class combines many such classes by using multiple inheritance.

#### SAFTd
In order to receive message passing interrups (MSIs) from the Hardware, an instance of SAFTd driver is needed. 
The name "SAFTd" is kept for backwards compatibility with older saftlib versions, in order to keep the user facing API stable.
A better name would be EtherboneSocket, because it encapsulates an etherbone::Socket together with some additional functions.
SAFTd provides:
 - An instance of an etherbone::Socket with an eb_slave device connected to it in order to receive MSIs
 - Redistribution of incoming MSIs to callback functions
 - A container of TimingReceiver objects (std::vector<std::unique_ptr<TimingReceiver> >). 




