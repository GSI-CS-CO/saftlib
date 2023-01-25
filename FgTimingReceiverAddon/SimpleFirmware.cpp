#include "SimpleFirmware.hpp"

SimpleFirmware::SimpleFirmware(saftlib::TimingReceiver *timing_receiver)
	: tr(timing_receiver)
{
	object_path = tr->getObjectPath();
	object_path.append("/simple-fw");

	saftlib::Mailbox *mbox = static_cast<saftlib::Mailbox*>(tr);
	const uint32_t CPU_MSI=0x0;
	msi_adr = tr->getSAFTd().request_irq(*mbox, std::bind(&SimpleFirmware::receive_hw_msi,this, std::placeholders::_1));
	cpu_msi_slot  = mbox->ConfigureSlot(CPU_MSI);
	host_msi_slot = mbox->ConfigureSlot(msi_adr);

	std::cerr << "cpu_msi_slot index " << cpu_msi_slot->getIndex() << std::endl;
	std::cerr << "host_msi_slot index " << host_msi_slot->getIndex() << std::endl;

	cnt = 0;
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
	saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&SimpleFirmware::send, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);
}

std::map<std::string, std::string> SimpleFirmware::getObjects() {
	std::map<std::string, std::string> result;
	result["simple-fw"] = object_path;
	return result;
}