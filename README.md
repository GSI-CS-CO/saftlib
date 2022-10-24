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

