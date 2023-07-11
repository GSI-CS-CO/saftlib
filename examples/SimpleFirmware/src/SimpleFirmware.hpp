#ifndef SIMPLE_FIRMWARE_HPP_
#define SIMPLE_FIRMWARE_HPP_

#include <vector>

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <TimingReceiverAddon.hpp>

#include <sigc++/sigc++.h>
/// @brief a very simple LM32 firmware driever that can be attached to a TimingReciver driver
/// 
/// In its constructor the firmware binary is loaded on the CPU with index 0.
/// And initializes the MailBox hardware to send MSIs to LM32 and host system.
/// The driver provides only one method "start" and one signal "MSI".
/// Initially, the driver does nothing.
/// After start is called, the driver will periodically send an MSI to the LM32 firmware.
/// The LM32 does nothing but wait for an MSI to arrive. It reacts to the MSI by sending 
/// another MSI back to the host system.
/// Ther is a command line tool "simple-firmware-ctl" which will will call the start method
/// and then waits for MSIs from the LM32.
namespace saftlib {
	class SimpleFirmware : public TimingReceiverAddon
	{
		std::string object_path;
		saftlib::SAFTd *saftd;
		saftlib::TimingReceiver *tr;
		std::vector<saftbus::SourceHandle> timeout_sources;

		/// Mailbox::Slot represents a configures Mailbox slot in hardware
		std::unique_ptr<saftlib::Mailbox::Slot> cpu_msi_slot;
		std::unique_ptr<saftlib::Mailbox::Slot> host_msi_slot;

		/// Remembers the address from register_irq in the constructor 
		/// and automatically releases it in the destructor
		std::unique_ptr<saftlib::IRQ> msi_adr;

		/// This is just a counter that is incremented each time an MSI is sent to the LM32 firmware.
		/// The current value of cnt is sent as part of the MSI data.
		int cnt;

		/// @brief The function is called whenever an MSI from hardware to host has the address 
		/// retured by "request_irq".
		void receive_hw_msi(uint32_t value);
		bool send_msi_to_hw();
	public:
		SimpleFirmware(saftlib::SAFTd *sd, saftlib::TimingReceiver *tr);
		~SimpleFirmware();

		/// @brief This is used by the TimingReceiver object that this Driver class is attached
		/// to register the service.
	    std::string getObjectPath();

		/// This function must be implemented by all TimingReceiverAddon classes so that the TimingReceiver
		/// service can report information (interface name, object name, object_path) about this child service to users.
		std::map< std::string , std::map< std::string, std::string > > getObjects();

		/// @brief Each call to this function will cause send_msi_to_hw() to be called every 100 ms. 
		/// Successive calls to this functions will add ever more periodic calls.
	    // @saftbus-export
	    void start();

	    /// @brief This will be emitted when an MSI from the hardware arrives 
	    /// (i.e. the callback function receive_hw_msi is called)
	    /// Users of the class (or the corresponding Proxy class) can connect to this signal
	    /// in order to receive the MSIs.
	    // @saftbus-export
	    sigc::signal<void, uint32_t> MSI;
	};
}

#endif
