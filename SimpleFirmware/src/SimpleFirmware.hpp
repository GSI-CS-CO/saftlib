#ifndef SIMPLE_FIRMWARE_HPP_
#define SIMPLE_FIRMWARE_HPP_

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <TimingReceiverAddon.hpp>

#include <sigc++/sigc++.h>
namespace saftlib {
	class SimpleFirmware : public TimingReceiverAddon
	{
		std::string object_path;
		saftlib::SAFTd *saftd;
		saftlib::TimingReceiver *tr;
		int cnt;
		saftbus::SourceHandle timeout_source;

		std::unique_ptr<saftlib::Mailbox::Slot> cpu_msi_slot;
		std::unique_ptr<saftlib::Mailbox::Slot> host_msi_slot;
		eb_address_t msi_adr;

		void receive_hw_msi(uint32_t value);
		bool send();
	public:
		SimpleFirmware(saftlib::SAFTd *sd, saftlib::TimingReceiver *tr);
		~SimpleFirmware();
	    std::string getObjectPath();

		std::map< std::string , std::map< std::string, std::string > > getObjects();


	    // @saftbus-export
	    void start();

	    // @saftbus-export
	    sigc::signal<void, uint32_t> MSI;
	};
}

#endif
