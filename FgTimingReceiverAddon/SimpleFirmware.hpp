#ifndef SIMPLE_FIRMWARE_HPP_
#define SIMPLE_FIRMWARE_HPP_

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>
#include <TimingReceiverAddon.hpp>

#include <sigc++/sigc++.h>

class SimpleFirmware : public saftlib::TimingReceiverAddon
{
	std::string object_path;
	saftlib::TimingReceiver *tr;
	int cnt;


	std::unique_ptr<saftlib::Mailbox::Slot> cpu_msi_slot;
	std::unique_ptr<saftlib::Mailbox::Slot> host_msi_slot;
	eb_address_t msi_adr;

	void receive_hw_msi(uint32_t value);
	bool send();
public:
	SimpleFirmware(saftlib::TimingReceiver *tr);
    std::map<std::string, std::string> getObjects();


    void start();

    sigc::signal<void, uint32_t> MSI;
};


#endif
