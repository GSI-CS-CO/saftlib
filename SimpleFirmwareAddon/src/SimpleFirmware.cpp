#include "SimpleFirmware.hpp"

namespace saftlib {

SimpleFirmware::SimpleFirmware(saftlib::SAFTd *sd, saftlib::TimingReceiver *timing_receiver)
	: saftd(sd)
	, tr(timing_receiver)
{
	object_path = tr->getObjectPath();
	object_path.append("/simple-fw");

	saftlib::Mailbox *mbox = static_cast<saftlib::Mailbox*>(tr);
	const uint32_t CPU_MSI=0x0;
	msi_adr = saftd->request_irq(*mbox, std::bind(&SimpleFirmware::receive_hw_msi,this, std::placeholders::_1));
	cpu_msi_slot  = mbox->ConfigureSlot(CPU_MSI);
	host_msi_slot = mbox->ConfigureSlot(msi_adr);

	std::cerr << "cpu_msi_slot index " << cpu_msi_slot->getIndex() << std::endl;
	std::cerr << "host_msi_slot index " << host_msi_slot->getIndex() << std::endl;


	// std::map<std::string , std::string> objects;
	// objects["simple_fw"] = object_path;
	// tr->addInterfaces("SimpleFirmware", objects);

	cnt = 0;
}

SimpleFirmware::~SimpleFirmware()
{
	std::cerr << "~SimpleFirmware() " << std::endl;
	saftbus::Loop::get_default().remove(timeout_source);
	saftd->release_irq(msi_adr);
}


void SimpleFirmware::receive_hw_msi(uint32_t value)
{
	MSI.emit(value);
}

bool SimpleFirmware::send()
{
	std::cerr << "triggerMSI: " << ++cnt << std::endl;

	cpu_msi_slot->Use((host_msi_slot->getIndex()<<16)|cnt);

	return true;
}

void SimpleFirmware::start() {
	timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&SimpleFirmware::send, this), std::chrono::milliseconds(100), std::chrono::milliseconds(100)
		);
}

std::string SimpleFirmware::getObjectPath() {
	return object_path;
}

std::map< std::string, std::string> SimpleFirmware::getObjects()
{
	std::map< std::string, std::string> result;
	result["simple-fw"] = object_path;
	return result;
}


}