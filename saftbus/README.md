# Saftbus: an interprocess communication library

## History

saftlib was originally written using D-Bus as inter process communication libray.
D-Bus was later replaced with a more real-time friendly system called saftbus.
For this reason, saftbus shares some properties with D-Bus:
  - a deamon running in the background maintains a list of available services
  - services offer an interface which consists of functions, properties (setter/getter), and signals
  - multiple processes can share services and use the provided interface
It differs from D-Bus in the following points
  - services cannot be added by processes connecting to the daemon, they have to be added by dynamically loading a plugin (shared library) into the daemon
  - Services are only usable if the signature of the interface is known. Introspection is not yet possible. 

## Features
  - an executable that should be run in the background (daemon) that provides services which can be accessed through a UNIX domain socket 
  - new services can be added by loading plugins at startup or during runtime of the daemon
  - a library and a code generator that facilitates developments of plugins for the daemon
  - a command line tool to control the daemon, e.g. load/unload new plugins or remove service objects
  - a deterministic memory allocator for realtime applications

## Environment varialbles 
  - **SAFTBUS_SOCKET_PATH** : determines the location of the UNIX domain socket in the file system. Default ist **/var/run/saftbus/saftbus**

## User guide
### Use saftlib in an application

the saftlib-plugin installs a pkg-config file. In order to compile an application, use the following:

    g++ `pkg-config saftlib --libs --cflags` -o application application.cpp
The following is an introduction of how to develop saftbus plugins.

### Develop saftbus plugins
Typcial use of saftbus is, to run saftbusd and load a custom plugin to provide custrom services and use custom programs that communicate with the services provided by the plugin.

In order to write a saftbus servcie, one has to create a C++ class. There are no restrictions on how to write the class. Signals can be added by using either std::function or sigc::signal. Inheritance (also multiple inheritance) is allowed. In order to expose a signal/method as a service interface that can be used by other processes, the comment **// @saftbus-export** has to appear in the source code in the line above the signal/method declaration of the class. 
A very simple plugin can be found in the [examples/ex01_simple_dice](examples/ex01_simple_dice) directory. The Dice class has one exposed method **throw** and one signal **result**. By running the class definition header file through the [saftbus-gen](../saftbus-gen/README.md) tool, some boilerplate code is generated:
  - A proxy class that has all the exposed methods
  - A service class that can be instanciated with a pointer to the actual driver class instance. 
The service class is compiled into a shared library wich can be loaded as plugin into the saftbusd daemon. 
The proxy class is compiled into a library, that programs can use to communicate with instances of the service class.

