# Simple Event snoop Tool

The configuration of the ECA always follows these steps:
  - Create a Proxy of the desired ECA output channel (aka action sink). In this case it is the SoftwareActionSink_Proxy. 
  - Create a new Condition on the SoftwareActionSink, the return value of the function is the object path of the newly created condition.
  - Create a SoftwareCondition_Proxy object for the newly created condition.
  - Connect the callback function to the "SigAction" signal of the SoftwareCondition_Proxy.
  - Enter a loop and wait for signals.

The relevant classes for this example are (you may need to run doxygen for the links to work)
  - [saftlib::SAFTd](../../html/classsaftlib_1_1SAFTd.html)
  - [saftlib::TimingReceiver](../../html/classsaftlib_1_1TimingReceiver.html)
  - [saftlib::softwareActionSink](../../html/classsaftlib_1_1SoftwareActionSink.html)
  - [saftlib::softwareCondition](../../html/classsaftlib_1_1SoftwareCondition.html)

This example comes with a simple, hand-written [makefile](makefile). 

## Using saflib Proxy objects (requires a running saftbusd with the libsaft-service.so plugin attached)
The program [saft-snoop-saftbus.cpp](saft-snoop-saftbus.cpp) will be described in the following.
### Saftlib Proxy header files
This program interacts with the hardware through instances of driver classes that are running as services on a saftbus daemon.
The intended way to do this is to use the provided proxy class that comes with every driver class.
In previous versions of saftlib, the proxy headers were named `SAFTd.h`, `TimingReceiver.h`, and so on. 
These headers will still work (compatibility headers are included in the installation of saftlib).
```C++
#include <SAFTd_Proxy.hpp>                // or <SAFTd.h>
#include <TimingReceiver_Proxy.hpp>       // or <TimingReceiver.h>
#include <SoftwareActionSink_Proxy.hpp>   // or <SoftwareActionSink.h>
#include <SoftwareCondition_Proxy.hpp>    // or <SoftwareCondition.h>
#include <CommonFunctions.h>
```
The last header `CommonFunctions.h` contains some convenience definitions, such as the `saftlib::wait_for_signal()` function. 
This header is automatically included when using the compatibility headers (such as `<SAFTd.h>`), so that older programs should compile without any changes.
### Other header files needed for the program
Whatever the program needs in addition.
```C++
#include <memory>
#include <iostream>
```
### Signal callback function
The signal callback function parameters should match the parameters of the signal definition in the Proxy header file (`sigc::signal<void, uint64_t, uint64_t, saftlib::Time, saftlib::Time, uint16_t> SigAction;`).
More parameters can be added, but then `sigc::bind` must be used when the function is connected to the signal.
In this case, the event information is printed to stdout.
```C++
void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
  std::cout << "event " << event << " " 
            << "param " << param << " " 
            << "deadline " << deadline.getTAI() << " "
            << "executed " << executed.getTAI() << " " 
            << "flags " << flags << " "
            << std::endl;
}
```
### Main function
The command line arguments need are parsed. 
```C++
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
```
### Create proxy objects 
Proxy objects are created. This is done by providing the object path of the desired service to the create method.
It is possible to directly create a proxy to any service if the object path is known, but it is best practice to 
always start with a SAFTd_Proxy (that doesn't need an object path, because it is always the same fixed path).
Then ask SAFTd_Proxy for object paths of child services and uses these to create the proxy objects. 
The programs become more flexible and more reusable this way.
To setup the host facing software action channel of the ECA, the `NewSoftwareActionSink` method of the TimingReceiver_Proxy
must be called. 
This function creates a SowftwareActionSink service running on the saftbus and returns the object path of it which can be used to create a SoftwareActionSink_Proxy object.
This can be used to create conditions in the ECA, and again, a service is created for that condition and the object path for it is returned.
```C++
  auto saftd = saftlib::SAFTd_Proxy::create();
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[argv[1]]);
  auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
  auto softwareActionSink          = saftlib::SoftwareActionSink_Proxy::create(softwareActionSink_obj_path);
  auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);
```
### ECA setup and connect callback function to signal
Finally, a SoftwareCondition_Proxy object can be created, and the callback function can be attached to the `SigAction` signal.
The condition proxy allows to configure the condition in an application specific way. In this case we want to display all events, even if they are late or early.
```C++
  auto sw_condition                = saftlib::SoftwareCondition_Proxy::create(condition_obj_path);
  sw_condition->setAcceptEarly(true);
  sw_condition->setAcceptLate(true);
  sw_condition->setAcceptDelayed(true);
  sw_condition->setAcceptConflict(true);
  sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));
```
### Wait for signals
The program has to wait for signals from the services. 
This is done by calling the `wait_for_signal()` function, which is idle until a signal from the saftbus daemon arrives. 
It will redistribute the signal parameters to all connected callback functions.
```C++
  for (;;) {
    //saftbus::Loop::get_default().iteration(true); 
    saftlib.wait_for_signal();
  }

  return 0;
}
```

## Using saftlib driver classes directly (standalone mode)
The program [saft-snoop-standalone.cpp](saft-snoop-standalone.cpp) will be described in the following. 
It is very similar to the program with proxy objects, and only differs in object creation. 
In this case the driver classes are used directly, but because proxy classes have an identical interface, the usage of proxy and driver objects is identical.
### Saftlib driver header files
The header files of the driver classes need to be included.
```C++
#include <SAFTd.hpp>
#include <SAFTd_Service.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>
```
The `saftbus/server.hpp` is only needed when this program wants to provide driver classes as services to other programs. 
If it is the only program that needs hardware access, this is not necessary.
The `saftbus/loop.hpp` is needed because the saftbus event loop is used in the driver classes, and this loop must be iterated somewhere in the program.
```C++
#include <saftbus/server.hpp>
#include <saftbus/loop.hpp>
```
The rest of the program is identical to the previous version above...
```C++
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
```
... until this point where (optionally, when `saftubs/server.hpp` was included), the following lines can be added to add saftbus server functionality to the program.
```C++
  // the next lines makes a fully functional saftd out of the program 
  saftbus::ServerConnection server_connection;
  saftbus::Container *container = server_connection.get_container();
  saftlib::SAFTd saftd(container); 
  std::unique_ptr<saftlib::SAFTd_Service> saftd_service(new saftlib::SAFTd_Service(&saftd));
  container->create_object("/de/gsi/saftlib", std::move(saftd_service));
  // end of saftd part
```
The other difference to the previous version of the program is how the objects are created. 
No object path is used to create them. 
The SAFTd object is created by instantiating one. 
The TimingReceiver is pulled out of `saftd` with `getTimingReceiver` and the SoftwareActionSink is pulled out of `tr` with the `getSoftwareActionSink` method. 
How exactly an object is created, but the general concept is that child objects are owned an managed by their parent object and there is a function that allows to get a pointer to the child.
```C++

  //saftlib::SAFTd saftd; // <- this is needed instead when no server functionality should be provided
  auto tr_obj_path                 = saftd.AttachDevice("tr0", argv[1], 100);                  
  saftlib::TimingReceiver* tr      = saftd.getTimingReceiver(tr_obj_path);                     
  auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");                            
  auto softwareActionSink          = tr->getSoftwareActionSink(softwareActionSink_obj_path);   
  auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);  
  auto sw_condition                = softwareActionSink->getCondition(condition_obj_path);     
```
Using the objects is identical to the proxy version.
```C++
  sw_condition->setAcceptEarly(true);
  sw_condition->setAcceptLate(true);
  sw_condition->setAcceptDelayed(true);
  sw_condition->setAcceptConflict(true);

  sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));
```
Waiting for events is different. Instead of waiting for signals from saftbus daemon, the main loop has to be iterated.
```C++
  for (;;) {
    saftbus::Loop::get_default().iteration(true); 
  }

  return 0;
}
```