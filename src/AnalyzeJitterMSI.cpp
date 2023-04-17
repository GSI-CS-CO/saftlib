#include "AnalyzeJitterMSI.hpp"

namespace saftlib {

AnalyzeJitterMSI::AnalyzeJitterMSI(saftlib::SAFTd *sd, saftlib::TimingReceiver *timing_receiver)
	: saftd(sd)
	, tr(timing_receiver)
{
	// stop the cpu, write firmware and reset cpu
	tr->SafeHaltCpu(0);
	std::string firmware_bin(DATADIR "/firmware/mj-firmware.bin");
	tr->WriteFirmware(0, firmware_bin);
	tr->CpuReset(0);

	// make up an object path, since we are a child of a TimingReceiver our object_path should 
	// start with its object_path, appended by a fitting name for our driver
	object_path = tr->getObjectPath();
	object_path.append("/mj-firmware");

	// In order to simplify the use of the Mailbox driver (from which TimingReceiver is derived)
	// create a Mailbox pointer.
	saftlib::Mailbox *mbox = static_cast<saftlib::Mailbox*>(tr);
	// This is the (fixed) MSI-slave address of the LM32 cluster. 
	// It cannot be read from the hardware (but is known for any given hardware)
	const uint32_t CPU_MSI=0x0; 
	// Since Mailbox is an MsiDevice (i.e. an MSI master), it can be used to call 
	// SAFTd->request_irq. Any MSI with the address equal to "msi_adr" will now be 
	// redirected the the connected callback function "receive_hw_msi"
	msi_adr = saftd->request_irq(*mbox, std::bind(&AnalyzeJitterMSI::receive_hw_msi,this, std::placeholders::_1));

	// configure one Mailbox slot with the CPU_MSI address
	cpu_msi_slot  = mbox->ConfigureSlot(CPU_MSI);
	// configuer another Mailbox slot with the address returned by request_irq, 
	// which is now associated with callback "receive_hw_msi"
	host_msi_slot = mbox->ConfigureSlot(msi_adr);

	// std::cerr << "cpu_msi_slot index " << cpu_msi_slot->getIndex() << std::endl;
	// std::cerr << "host_msi_slot index " << host_msi_slot->getIndex() << std::endl;

	cnt = 0;
}

AnalyzeJitterMSI::~AnalyzeJitterMSI()
{
	// Remove all TimoutSources from the event loop.
	for (auto &source: timeout_sources) {
		saftbus::Loop::get_default().remove(source);
	}
	// Release_irq allows the MSI address to be reused by another driver.
	saftd->release_irq(msi_adr);
}



void AnalyzeJitterMSI::receive_hw_msi(uint32_t value)
{
	// Just send a signal to whoever is connected to it.
	// If this Driver class runs in a saftbus-daemon, the signal will be
	// redirected over saftbus to AnalyzeJitterMSI_Proxy objects.
	MSI.emit(value);
}

// send an MSI to the LM32 cluster
bool AnalyzeJitterMSI::send_msi_to_hw(uint32_t value)
{
	// keep track of the number of send MSIs (cnt) in order to see if they arrive ( in the correct order)
	// std::cerr << "triggerMSI: " << ++cnt << std::endl;
	// Use the Mailbox::Slot. The use function will cause the Mailbox to send an MSI
	// with the pre-configured address and data equal to the argument of Use(<data>).
	// In this case the pre-configured address targets the LM32-cluster.
	cpu_msi_slot->Use((host_msi_slot->getIndex()<<24) | value);
	// returning true here means that the TimeoutSource is not disconnected hereafter.
	return true;
}

void AnalyzeJitterMSI::start(uint32_t delay) {
	std::cerr << "start" << std::endl;
	send_msi_to_hw(delay);
	// register a periodic TimeoutSouce to call "send_msi_to_hw" every 100 ms
	// timeout_sources.push_back(saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
	// 		std::bind(&AnalyzeJitterMSI::send_msi_to_hw, this), std::chrono::milliseconds(100), std::chrono::milliseconds(100)
	// 	));
}
void AnalyzeJitterMSI::stop() {
	std::cerr << "stop" << std::endl;
	send_msi_to_hw(0);
}


std::string AnalyzeJitterMSI::getObjectPath() {
	return object_path;
}

std::map< std::string, std::map< std::string, std::string> > AnalyzeJitterMSI::getObjects()
{
	std::map< std::string, std::map< std::string, std::string> > result;
	result["AnalyzeJitterMSI"]["mj-firmware"] = object_path;
	return result;
}


}