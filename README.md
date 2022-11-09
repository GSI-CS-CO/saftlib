# Saftbus

The saftbus backage provides 
	* a deamon that provides services which can be accessed through a UNIX domain socket. New services can be added by plugins at startup or during runtime of the daemon.
	* a library and a code generator that facilitates developments of plugins for the daemon.
	* a command line tool to control the daemon.

The aim is to make the deamon itself realtime friendly. That means
	* heap allocations only at startup of the daemon or when loading a service plugin, but not during normal operation. Creating a timing Source violates that at the moment, which has to be fixed!
	* no mutex locks in the daemon thread. At the moment the daemon has only a single thread.

## user guide

## plugin developer guide

### Overview
The daemon (saftbusd) is not useful by itself. Plugins need to be loaded to make it functional.
This section describes general rules for plugin developers.

Plugins are defined as C++ classes, that have some tagged public methods and some tagged public 
signals (std::function or sigc::slot). These tagged classes can be processed by saftbus-gen which
generates a matchin Service and Proxy class. All Service classes need to be bundled into a shared 
library together with an implementation of two (extern "C") functions: 
	create_services and destroy_service

saftbusd can load such a library either on startup or during runtime with saftbus-ctl.
When a library is loaded, the create_services function is called once. It returns a 
std::vector of pairs of object_path (an std::string) and Service. All these services are installed on 
the saftbus and are addressable by their object path. Services can create and install new 
Services. As a general rule, the instance of the driver class owns the instances of all driver 
classes it creates. The ownership of the Service classes is transferred to saftbusd after construction.

With a little care, a plugin can be written in a way to make the driver class usable in standalone 
applications with almost the same syntax as in IPC applications. 

### Steps and rules to convert a C++ class library into a safbus plugin
	- All or some of the classes may have some methods that should be made availave via saftbus Service. These classes are the driver classes of the service objects. To create the service class sourcecode, annotate all exported methods with "// @safbus-export" and call saftbus-gen on the driver class header file. The output of saftbus-gen are four C++ source files per driver class. A makefile rule to do this looks like this

			%_Service.cpp %_Service.hpp %_Proxy.cpp %_Proxy.hpp: %.hpp
				saftbus-gen $< -o src
	- Add a source file "create_service.cpp" with an extern "C" function "create_services(saftbus::Container *container)". This function should create instances of the toplevel classes. For each instance, a corresponding service object needs to be created on the heap (constructor takes a raw pointer to the driver class instance), encapsulated in a std::unique_ptr, and moved to the container by calling "container->create_object(object_path, service)".
	- If a driver class creates child objects with services on saftbus, it also needs to create the child services and add them to the saftbus::Container just like the "create_services" function. If the child driver classes should be destroyed whenever their corresponding service object is removed from saftbus, then a destruction_callback should be passed to the service constructor.
	- If there is a possibility to destroy any dirver class without destruction of the corresponding service object, the destructor of the driver class must make shure that the corresponding service object is removed from saftbus::Container (by calling saftbus::Container::remove_object(const std::string &object_path)). In this case, driver classes need to store the object path to their corresponding service object.
	- Object paths must reflect the parent child relationship correctly (if there is any relationship, e.g. child driver instances are stored inside a container of the parent driver instance). This is important if any driver makes use of the ownership feature. saftbus::Container::remove_object uses the object path to check if the removal candidate has children with a different owner. If that is the case removal is not possible. For example objects "/parent" and "/parent/child" are available on saftbus. If "/parent/child" is owned by client 4, and client 5 calls remove_object("/parent"), an exception will be thrown and the removal will not happen. If the child driver class can operate independently from the parent driver class the objects paths should be "/parent" and "/child" instead.


## developer guide

### Concepts
#### general coding style
The Pimpl idom is used for some classes.
Classes that use the Pimpl idom look like this:

		class Classname {
				struct Impl; std::unique_ptr<Impl> d;
		public:
				// public interface
		}

For these classes, all private members are defined in the Classname::Impl structure in the .cpp file.


Server: 
 * A process that runs a Loop and has a ServerConnection object.

ServerConnection:
 * creates a socket in the file system. The default path is "/var/run/mini-saftlib/saftbus".
 * owns all IPC file descriptors and makes sure that close() is called whenever any file descriptor is not used anymore.
	 Especially if a client disconnects and a Service still has signal file descriptors to that client, these are closed.
 * maintains a container of Service objects.
 * maintains a list of ConnectedClients.

Client: 
 * A process that uses the Server socket to establish a private connection to the server.
	 This connection uses a separate socket-pair that does not appear in the file system.
 * Can instantiate Proxy objects that are local representations of Service objects on the Server.

Service:
 * A service provides methods and can emit signals.
 * Can implement multiple Interfaces.

Interface: 
 * A set of methods and signals.

ServiceObject (or just Object): 
 * an instance of a Service of which a Poxy object can be created.
 * has an object path which is a unique string identifier. "/de/gsi/saftlib"

