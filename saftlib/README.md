# Saftlib v3 - Simplified API for Timing

GSI timing receiver hardware includes a large collection of slave devices.
Saftlib provides a user-friendly software interface for controlling these devices.
In a typcal use case, one process (saftbusd) shares access to these hardware resources on behalf of multiple client programs.
Clients connect to saftbusd using a C++ library of proxy objects which represent the shared resources.
The user API of for shared resources hides device register access and provides high-level functionality.

# Table of Contents

- [Saftlib](#saftlib)
  * [API documentation](#api-documentation)
  * [Compile and install](#compile-and-install)
    + [Etherbone](#etherbone)
    + [Quickstart](#quickstart)
  * [Version 3 changes](#version-3-changes)
  * [Saftlib user guide](#saftlib-user-guide)
    + [Start saftbusd and load the saftlib-service.so plugin](#start-saftbusd-and-load-the-saftlib-serviceso-plugin)
    + [Firmware drivers](#firmware-drivers)
      - [Burst generator](#burst-generator)
      - [Function generator](#function-generator)
    + [Developing software using saftlib Driver or Proxy classes](#developing-software-using-saftlib-driver-or-proxy-classes)
      - [Best practices](#best-practices)
      - [Limitations](#limitations)
      - [Shared access or exclusive access](#shared-access-or-exclusive-access)
      - [Examples](#examples)
        * [Example 1: A simple event snoop tool](#example-1--a-simple-event-snoop-tool)
        * [Example 2: A serial transceiver using an IO of a TimingReceiver](#example-2--a-serial-transceiver-using-an-io-of-a-timingreceiver)
    + [Develop plugins to add new services](#develop-plugins-to-add-new-services)
        * [Example 1: A simple LM32 firmware driver](#example-1--a-simple-lm32-firmware-driver)

# Saftlib

## API documentation

Generated with Doxygen from the master branch, the Saftlib Documentation is available here: [https://gsi-cs-co.github.io/saftlib](https://gsi-cs-co.github.io/saftlib).

## Compile and install

```bash
./autogen.sh
./configure
make install
```

Default installation directory prefix is `/usr/local`. It can be changed (for example to `$HOME/.local`), by running configure with the --prefix option like this
```bash
./configure --prefix=$HOME/.local
```

### Etherbone

You might encounter this error:

```
checking for etherbone >= 2.1.0... no
configure: error: Package requirements (etherbone >= 2.1.0) were not met:

Package 'etherbone', required by 'virtual:world', not found
```

To solve this, you need to provide a folder that contains an etherbone.pc file:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

### librtpi

The [Real-Time Priority Inheritance Library](https://gitlab.com/linux-rt/librtpi) is used in saftlib.
Please see [librtpi](https://git.gsi.de/saft/librtpi/) on how to install it.

You might encounter this error:

```
checking for librtpi >= 1.0.2... no
configure: error: Package requirements (librtpi >= 1.0.2) were not met:

Package 'librtpi', required by 'virtual:world', not found
```

To solve this, you need to provide a folder that contains an librtpi.pc file:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```
### Quickstart

If saftbusd (or any client program) is started by non root users, the socket directory (where the socket for inter process communication is located) should be changed by setting the SAFTBUS_SOCKET_PATH environment variable. For example

```bash
export SAFTBUS_SOCKET_PATH=/tmp/saftbus # only for non-root user
export LD_LIBRARY_PATH=$HOME/.local/lib # only if installation prefix was changed to $HOME/.local
saftbusd libsaft-service.so tr0:dev/wbm0 # this blocks the terminal. Interact with saftbusd from another terminal
```

The following command (in a different terminal) shows all services registered on saftbusd.

```bash
export SAFTBUS_SOCKET_PATH=/tmp/saftbus # only for non-root user
export LD_LIBRARY_PATH=$HOME/.local/lib # only if installation prefix was changed to $HOME/.local
saftbus-ctl -s
```

Using saftbusd like this is useful for tests and development. For use in production it is recommended to do a proper installation and launch saftbusd as a systemd unit.

## Version 3 changes

Version 3 of saftlib has major internal changes compared to version 2, but aims to keep the user API unchanged.

  - Rewrite of the inter process communication system ([saftbus](saftbus/README.md))
    - Better stability and performance.
    - Use fewer file descriptors.
    - Give up compatibility with Gio::Dbus-API for simpler code.
    - Give up xslt-based code generator, a code generator written in C++ is provided ([saftbus-gen](saftbus-gen/README.md)).
    - Give up on xml interface definition files, exported functions are identified with a special comment.
    - Allow to write driver code that can be used with and without inter process communication.
    - Provide a plugin mechanism to install/remove services at runtime for more flexible customization of TimingReceiver hardware, e.g. LM32 firmware and saftlib driver.
  - Startup of the daemon
    - A wild card character is allowed for the device name and etherbone-path: `saftbusd libsaft-service.so tr*:dev/wbm*` will attach all matching devices.
    - For USB devices the MSI polling period in ms can be specified after the etherbone device name separated by a colon. The following example specifies a MSI polling period of 15 ms: `saftbusd libsaft-service.so tr0:dev/ttyUSB0:15`
    - Command to start the services is `saftbusd libsaft-service.so tr0:dev/wbm0`  (a `saftd` script that wraps the call to saftbusd is provided, so `saftd tr0:dev/wbm0` like in version 2 is still possible).
    - Drivers for LM32 firmware (like burst-generator and function-generator) are not loaded by default. They need to be added explicitly when starting saftbusd (see [Firmware Drivers](#firmware-drivers)).
      - `saftbusd libsaft-servcie.so tr0:dev/wbm0 libfg-firmware-service.so tr0` if the function generator is needed on a SCU.
      - `saftbusd libsaft-service.so tr0:dev/wbm0 libbg-firmware-service.so tr0` if the burst generator is needed on a TimingReceiver.
  - Refactoring of the driver code (ECA, IOs, MSI)
    - The code is compiled into a plugin for saftbusd, i.e. it can be loaded into saftbusd at runtime and functionality can be extended with other plugins.
    - Better performance of the eb-source.
    - Removal of msi-source.
    - All major SDB devices on the hardware have a C++ class representation.
    - Simplified registration of MSI callbacks.
    - Simplified setup of the Mailbox device.
    - Programs can link against the driver library using proxy objects, or directly against the driver code for exclusive lower latency access to the hardware.
    - The documentation of the user facing API is generated with doxygen and can be found in [html/index.html](html/index.html) after running doxygen inside of the saftlib directory.
    - **user facing API is compatible to saftlib major version 2** with the following additions:
      - The clock generator driver remembers the setting of any IO that was configured.
      - ToggleActive and InactivateAll were added to the TimingReceiver (ECA) interface. These functions modify all owned conditions on all ActionSinks.

## Saftlib user guide

The package contains two libraries
  - **libsaft-service**: Driver classes and server side classes for inter process communication with the client side proxy classes. This library can be dynamically loaded into a running saftbusd.
  - **libsaft-proxy**: Client side proxy classes for inter process communication with the driver classes. Normally, user programs are developed against the Proxy classes.

And a set of command line tools to control and interact with attached hardware devices.
  - **saft-ctl**: Configure software action sinks on the ECA. Typically used to snoop for timing events or inject timing events into the ECA locally. Can also be used to attach and remove devices.
  - **saft-io-ctl**: Configure output action sinks on the ECA or configure or snoop on timing receiver inputs.
  - **saft-pps-gen**: Configure one pulse per second output signal on all outputs of the attached timing receiver. Typically used for tests.
  - **saft-scu-ctl**: Configure the SCU bus action sink of the ECA.
  - **saft-ecpu-ctl**: Configure the embedded CPU action sink of the ECA.
  - **saft-wbm-ctl**: Configure the wishbone master action sink of the ECA.
  - **saft-clk-gen**: Configure the clock generator of an output.
  - **saft-dm**: A program that can simulate a DataMaster by locally injecting events from a file into the ECA.
  - **saft-eb-fwd**: Report the etherbone forwarding device of an attached timing receiver. This is mainly used to access a USB-connected timing receivers with etherbone-tools while saftlib is already connected. This is needed because USB can only support one etherbone connection. Example: `eb-ls dev/ttyUSB0` becomes `eb-ls $(saft-eb-fwd tr0)` when saftlib is connected with `tr0:dev/ttyUSB0`.
  - **saft-gmt-check**:  Allows to check some functionalities of the General Machine Timing System (GMT).
  - **saft-uni**: A tool for on UNILAC specific features.
  - **saft-lcd**: Live Chain Display. This tool uses saftlib for on-line snooping and display of beam production chains.
  - **saft-roundtrip-latency**: A test program for latency measurements of the stack (including inter process communication)
  - **saft-standalone-roundtrip-latency**: A test program for latency measurements of the stack (without inter process communication)
  - **saft-burst-ctl**: Controls the burst generator LM32 firmware. The libbg-firmware-service plugin needs to be loaded before this tool can be used.
  - **saft-fg-ctl**: Controls the function generator LM32 firmware. The libft-firmware-service plugin needs to be loaded before this tool can be used.

### Start saftbusd and load the saftlib-service.so plugin

The most common use case for saftlib, is to be used over saftbus. A running saftbusd with the libsaft-servcie.so plugin is needed and a TimingReceiver_Service has to be attached to SAFTd. This can be achieve by launching saftbusd like this (assuming that a FAIR Timing Receiver is attached to the system under /dev/wmb0):

```bash
saftbusd libsaft-service.so tr0:dev/wbm0 tr1:dev/wbm1 tr2:dev/ttyUSB0
```

Wildcard character `*` is allowed, so the following line is equivalent to the previous one (if only `dev/wbm0` and `dev/wbm1` exist)

```bash
saftbusd libsaft-service.so tr*:dev/wbm* tr2:dev/ttyUSB0
```

Alternatively, saftbusd can be launched without any plugins and libsaft-service.so can be loaded later using saftbus-ctl

```bash
saftbus-ctl -l libsaft-service.so tr0:dev/wbm0 tr1:dev/wbm1 tr2:dev/ttyUSB0
```

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

If the libsaft-service plugin is running, additional devices can be attached using
```bash
saft-ctl tr5 attach dev/wbm5
```
Devices can be removed (only if none of their child services are owned by a client process) with
```bash
saft-ctl tr5 remove
```
The SAFTd service can be removed (only if none of their child services are owned by a client process) with
```bash
saft-ctl tr5 quit
```
The saftbus tool `saftbus-ctl` also allows to remove services. See ([saftbus](saftbus/README.md)) for details.

### Firmware drivers

Two firmware drivers are contained in the saftlib package.
They are be compiled by the build system into separate shared libraries (`libbg-firmware-service.so` and `libfg-firmware-service.so`) and are installed together with libsaft-service.so.

#### Burst generator

The burst generator service is not part of the libsaft-service.so.
The service has to be loaded as a plugin into saftbusd. The service requires a matching firmware running on one of the user LM32 cores.
The firmware binary is installed at `${prefix}/share/saftlib/firmware/burstgen.bin` and can be automatically loaded with the driver plugin.
When loading the driver plugin `libbg-firmware-service.so { <saftlib-device> [lm32-core-index] }` and optional lm32-core-index is provided, the plugin loader will write the firmware binary to the requested core index. There are no checks if another firmware is already running on that core.
The plugin can be loaded into a running saftbusd using saftbus-ctl, for example:

```bash
saftbus-ctl -l libbg-firmware-service.so tr0 0
```

This will install the driver on device tr0 after loading the firmware into lm32-core 0 on this device.

The plugin can be loaded when saftbusd is started, for example:

```bash
saftbusd libsaft-service.so tr0:dev/wbm0 libbg-firmware-service.so tr0 0
```

Before using the burst generator, the firmware has to be put into a working state with

```bash
saft-burst-ctl tr0 -i 1
```

#### Function generator

The function generator service is not part of libsaft-service.so.
The service has to be loaded as a plugin into saftbusd. The service requires a matching firmware running on one of the user LM32 cores.
SCU timing receivers are already preloaded with the function generator firmware binary. This service is meant to be used only on SCU timing receivers.
The plugin can be loaded into a running saftbusd using saftbus-ctl, for example:

```bash
saftbus-ctl -l libfg-firmware-service.so tr0
```

This will install the driver on device tr0.
By default, the driver will provide two services `/de/gsi/saftlib/tr0/fg-firmware` and the MasterFunctionGenerator interface `/de/gsi/saftlib/tr0/fg-firmware/master_fg` which bundles all available function generator channels behind one interface.
If individual channel interfaces are needed, a call to

```bash
saft-fg-ctl -d tr0 -si
```

Will initiate the firmware to rescan the available function generator channels and provide the FunctionGenerator interface for each of them. Calling

```bash
saft-fg-ctl -d tr0 -sm
```

Will initiate the firmware to rescan the available function generator channels and provide the default interface (MasterFunctionGenerator).

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

Both options are implemented in the simple event snoop tool of the [Examples](#examples) section.

#### Examples

##### Example 1: A simple event snoop tool

A simple program that can be used to monitor timing events.
The program configures the ECA to effectively call the function "on_action" whenever an event matches the specified event id.
There are two versions of this example, showing both, shared and exclusive access to the hardware.

[Example](examples/EventSnoopTool/README.md)

##### Example 2: A serial transceiver using an IO of a TimingReceiver

An example with multiple threads and different `saftbus::SignalGroups`.
It shows how to reuse existing conditions and react on the Destroy signal.

[Example](examples/SerialTransceiver/README.md)

### Develop plugins to add new services

##### Example 1: A simple LM32 firmware driver

TimingReceiver hardware provides LM32 soft-core CPUs. These can be programmed with use case specific firmware.
Saftlib3 allows to extend the functionality of the TimingReceiver driver with plugins.

[Example](examples/SimpleFirmware/README.md)
