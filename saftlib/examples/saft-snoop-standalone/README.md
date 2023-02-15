# Stand-alone saftlib application

The Driver classes in saftlib can be used directly in an application, without using inter process communication (IPC) with a daemon.
This implies:
  * The application has lower latency when talking to the hardware because it 
  * The normal saft-tools (saft-ctl, saft-io-ctl, etc.) do not work because they require a IPC server
  * However, it is possible to include IPC server fuctionality into the application. In this case, the application has fast access to hardware while still allowing other processes (e.g. saft-tools) to use the hardware.

saft-snoop-standalone is a minimalistc application to demonstrate how to write and build standalone saftlib applications, and how to provide saftd server functionality.

## Code snippet to add IPC server functionality (programm needs to be linked against -lsaftbus-server)

The following code creates a saftbus server with a SAFTd_Service ready to be used by other processes (saft-ctl, saft-io-ctl, ...), attaches a hardware device and obtains a pointer to that hardware device to be used in the program. The program will have better latency compared to IPC connected programs.

    #include <SAFTd.hpp>
    #include <SAFTd_Service.hpp>
    #include <TimingReceiver.hpp>
    #include <saftbus/server.hpp> // for saftbus::ServerConnection and saftbus::Container*
     
    saftbus::ServerConnection server_connection;                                               // creates and maintains the saftbus socket
    saftbus::Container *container = server_connection.get_container();                         // container manages the *_Service objects
    saftlib::SAFTd saftd(container);                                                           // an instance of the SAFTd driver class
    std::unique_ptr<saftlib::SAFTd_Service> saftd_service(new saftlib::SAFTd_Service(&saftd)); // an instance of a SAFTd_Service
    container->create_object("/de/gsi/saftlib", std::move(saftd_service));                     // insert SAFTd_Service into container
    auto tr_obj_path                 = saftd.AttachDevice("tr0", "dev/wbm0");                  // tell SAFTd driver to attach a hardware device
    saftlib::TimingReceiver* tr      = saftd.getTimingReceiver(tr_obj_path);                   // get a pointer to the attached hardware devcie


## Code snippet to exclusively use hardware without IPC server (programm can be linked against -lsaftbus-standalone)

    #include <SAFTd.hpp>
    #include <TimingReceiver.hpp>
     
    saftlib::SAFTd saftd;                                                           // an instance of the SAFTd driver class
    auto tr_obj_path                 = saftd.AttachDevice("tr0", "dev/wbm0");       // tell SAFTd driver to attach a hardware device
    saftlib::TimingReceiver* tr      = saftd.getTimingReceiver(tr_obj_path);        // get a pointer to the attached hardware devcie
