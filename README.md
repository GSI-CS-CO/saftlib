# Saftlib v3 - Simplified API for Timing

GSI timing receivers include a large collection of slave devices. Saftlib
provides a user-friendly software interface for controlling these slaves.
The interfaces provide high level functionality. 


## Version 3 changes

#### Rewrite of the inter process communication system ([saftbus](saftbus/README.md)) 
  - Better stability and performance.
  - Use fewer file descriptors.
  - Give up compatibility with Gio::Dbus-API for simpler code.
  - Give up xslt-based code generator, a code generator in C++ is provided ([saftbus-gen](saftbus-gen/README.md)).
  - Give up on xml interface definition files, exported functions are now annotated with a special comment to be exported.
  - Allow to write driver code that can be used with and without inter process communication.
  - Provide a plugin mechanism to install/remove services at runtime for more flexible customization of TimingReceiver hardware, e.g. LM32 firmware and saftlib driver.

#### Refactoring of the driver code (ECA, IOs, MSI)
  - The code is now compiled into a plugin for saftbus daemon, i.e. it can be loaded into a running saftbus-daemon and functionality can be extended with other plugins.
  - Better performance of the eb-source.
  - Removal of msi-source.
  - All major SDB devices on hardware have a C++ class representation.
  - Much simpler registration of MSI callbacks.
  - Much simpler use of the Mailbox device.
  - Programs can link against the driver library in the normal way, using proxy objects, or in standalone fashion for exclusive lower latency access to the hardware.
  - **user facing API is compatible to saftlib major version 2** with the following additions:
    - The clock generator driver remembers the setting of any IO that was configured.
    - ToggleActive and InactivateAll were added to the TimingReceiver (ECA) interface. These functions modify all owned conditions on all ActionSinks.
    - At startup, a wild card character is allowed for the device name and etherbone-path: `saftbusd libsaft-service.so tr*:dev/wbm*` will attach all matching devices.
    - Drivers for LM32 firmware (like burst-generator and function-generator) are not loaded by default. They need to be added explicitly when starting saftbusd (see [Firmware Drivers](#firmware-drivers)).
  - The documentation of the user facing API is generated with doxygen and can be found in [html/index.html](html/index.html) after running doxygen inside of the saftlib directory.

#### Installation
    - ./autogen.sh
    - ./configure 
    - make install

#### Startup of the daemon
  - **saftbusd libsaft-service.so tr0:dev/wbm0**  (instead of **saftd tr0:dev/wbm0** in saftlib version 2)
  - **saftbusd libsaft-servcie.so tr0:dev/wbm0 libfg-firmware-service.so tr0** if the function generator is needed on a SCU
  - **saftbusd libsaft-service.so tr0:dev/wbm0 libburst-generator-service.so tr0** if the burst generator is needed on a TimingReceiver

## Saftlib user guide

Saflib is a library and a set of tools to use and control the FAIR Timing Receiver hardware.
Saftlib provides
  - A C++ class library to access FAIR Timing Receiver Hardware. It can be used to configure the ECA channels, create conditions and receive software Interrupts from timing events. 
  - A library of proxy classes that can be used to share hardware resources by access through [saftbus](saftbus/README.md) via IPC (inter process communication).
  - A library that can be directly by stand alone programs.
  - A set of command line tools to control and interact with attached hardware devices.

### Start saftbusd and load the saftlib-service.so plugin

The most common use case for saftlib, is to be used over saftbus. A running saftbusd with the libsaft-servcie.so plugin is needed and a TimingReceiver_Service has to be attached to SAFTd. This can be achieve by launching saftbusd like this (assuming that a FAIR Timing Receiver is attached to the system under /dev/wmb0):

    saftbusd libsaft-service.so tr0:dev/wbm0 tr1:dev/wbm1 tr2:dev/ttyUSB0

Alternatively, saftbusd can be launched without any plugins and libsaft-service.so can be loaded later using saftbus-ctl

    saftbus-ctl -l libsaft-service.so tr0:dev/wbm0 tr1:dev/wbm1 tr2:dev/ttyUSB0

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
  libsaft-service.so

connected client processes:
  10 (pid=432932)
```


### Firmware drivers
Two firmware drivers are contained in the saftlib package. 
They are be compiled by the build system into separate shared libraries (`libbg-firmware-service.so` and `libfg-firmware-service.so`) and are installed together with libsaft-service.so.
#### Burst generator
The burst generator service is not part of the libsaft-service.so. 
The service has to be loaded as a plugin into saftbusd. The service requires a matching firmware running on one of the user LM32 cores.
The firmware binary is installed at `${prefix}/share/saftlib/firmware/burstgen.bin` and can be automatically loaded with the driver plugin.
When loading the driver plugin `libbg-firmware-service.so { <saftlib-device> [lm32-core-index] }` and optional lm32-core-index is provided, the plugin loader will write the firmware binary to the requested core index. Make sure that there is no required firmware running on that lm32-core. There are no checks if a firmware is already running.
The plugin can be loaded into a running saftbusd using saftbus-ctl, for example:
```
saftbus-ctl -l libbg-firmware-service.so tr0 0
```
This will install the driver on device tr0 after loading the firmware into lm32-core 0 on this device.

The plugin can be loaded when saftbusd is started, for example:
```
saftbusd libsaft-service.so tr0:dev/wbm0 libbg-firmware-service.so tr0 0
```

Before using the burst generator, the firmware has to be put into a working state with 
```
saft-burst-ctl tr0 -i 1
```


#### Function generator

### Developing software using saftlib Driver or Proxy classes

#### Best practices 
In order to get the best performance out of saftlib, users should follow some basic rules:
  - If a Proxy objects is used frequently int the program it should be stored instead of recreated if it is needed. For example, when new ECA conditions are frequently defined, the corresponding ActionSink Proxy should be stored instead of calling ActionSink::create() whenever an ActionSink_Proxy is needed. Reason: Creating a Proxy object has some overhead, it loads the program as well as the saftbusd server. 
  - Configuring the ECA takes relatively long. If many conditions should be changed, the fastest way is to create a complete new set of conditions in an inactive state, then call ToggleActive which will deactivate all active conditions and activate all inactive conditions. Then remove all inactive conditions. This results in only one reconfiguration of the ECA hardware.

#### Limitations
There are some limitations that users should be aware of.
  - It is not possible to create or destroy Proxy objects on a saftbus::SignalGroup inside a Signal callback function from the same saftbus::SignalGroup.


#### Shared access or exclusive access  
There are two fundamentally different ways of writing code that uses saftlib:
  - The classes from saftlib can be used remotely through Proxy objects, this is shown in the first code example. In this case many processes can share the hardware resources at the same time. A saftbusd server has to be available with the saftd-service plugin loaded and a TimingReceiver hardware attached. This is probably the most common use case.
  - Alternatively, the driver classes can also be used directly for lower latency (standalone, without running saftbusd). In this case the program connects directly to the hardware and no saftbusd server can run connect to the same hardware at the same time. But the program can be written in a way to provide the same functionality as saftbusd. This option might be useful when extra low latency in the hardware communication is needed.

#### Examples 

##### Example 1: [A simple event snoop tool](examples/EventSnoopTool/README.md)

A simple program that can be used to monitor timing events.
The program configures the ECA to effectively call the function "on_action" whenever an event matches the specified event id.
There are two versions of this example, showing both, shared and exclusive access to the hardware.

##### Example 2: [A serial transceiver using an IO of a TimingReceiver](examples/SerialTransceiver/README.md)

An example with multiple threads and different `saftbus::SignalGroups`.
It shows how to reuse existing conditions and react on the Destroy signal.

### Develop plugins to add new services

##### Example 1: [A simple LM32 firmware driver](examples/SimpleFirmware/README.md)

TimingReceiver hardware provides LM32 soft-core CPUs. These can be programmed with use case specific firmware.
Saftlib3 allows to extend the functionality of the TimingReceiver driver with plugins.





