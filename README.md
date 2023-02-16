# Saftlib - Simplified API for Timing

GSI timing receivers include a large collection of slave devices. Saftlib
provides a user-friendly software interface for controlling these slaves.
The interfaces provide high level functionality. 

### Version 3 changes
#### Rewrite of the inter process communication system ([saftbus](saftbus/README.md)) 
  - better stability and performance
  - use fewer file descriptors
  - give up compatibility with Gio::Dbus-API for simpler code 
  - give up xslt-based code generator, a code generator in C++ is provided (saftbus-gen subdirectory)
  - give up on xml interface definition files, exported functions are now annotated with a special comment to be exported
  - allow to write driver code that can be used with and without inter process communication
  - provide a plugin mechanism to install/remove services at runtime for more flexible customization of TimingReceiver hardware, e.g. LM32 firmware and saftlib driver

#### Major rearrangement of the ECA/IO/MSI driver code ([saftlib](saftlib/README.md)) 
  - is saftbus plugin for saftbus daemon, i.e. it can be loaded into a running saftbus-daemon and funcionality can be extended with other plugins
  - better performance of the eb-source
  - removal of msi-source
  - all major SDB devices on hardware hava a C++ class representation 
  - much simpler registration of MSI callbacks
  - much simpler use of the Mailbox device
  - executables can be linked against the driver library in the normal way, using proxy objects, or in standalone fashion for exclusive lower latency access to the hardware
  - **user facing API is compatible to saftlib major version 2** with the following additions:
    - The clock generator driver remembers the setting of any IO that was configured.
    - ...  (?)
  - The documentation of the user facing API is generated with doxygen and can be found in saftlib/html/index.html after running doxygen inside of the saftlib directory

#### Startup of the daemon has changed 
  - **saftbusd libsaftd-service.so tr0:dev/wbm0**  (instead of **saftd tr0:dev/wbm0** in saftlib version 2)
  - **saftbusd libsaftd-servcie.so tr0:dev/wbm0 libfg-firmware-service.so tr0** if the function generator is needed on a SCU
  - **saftbusd libsaftd-service.so tr0:dev/wbm0 libburst-generator-service.so tr0** if the burst generator is needed on a TimingReceiver

