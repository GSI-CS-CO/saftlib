#include <SAFTd.hpp>
#include <SAFTd_Service.hpp>
#include <TimingReceiver.hpp>
#include <SoftwareActionSink.hpp>
#include <SoftwareCondition.hpp>

#include <saftbus/client.hpp>
#include <saftbus/server.hpp>

#include <memory>
#include <iostream>

void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
	std::cout << "event " << event << " " 
	          << "param " << param << " " 
	          << "deadline " << deadline.getTAI() << " "
	          << "executed " << executed.getTAI() << " " 
	          << "flags " << flags << " "
	          << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc != 5) {
		std::cerr << "usage: " << argv[0] << " <etherbone-path> <id> <mask> <param>" << std::endl;
		return 0;
	}

	uint64_t snoop_args[3]; 
	uint64_t &id    = snoop_args[0];
	uint64_t &mask  = snoop_args[1];
	uint64_t &param = snoop_args[2];
	for (int i = 2; i < 5; ++i) {
		std::istringstream argin(argv[i]);
		argin >> snoop_args[i-2];
		if (!argin) {
			std::cerr << "cannot read snoop argument from " << argv[i] << std::endl;
			return 1;
		}
	}


	// the next lines makes a fully functional saftd out of the program 
	saftbus::ServerConnection server_connection;
	saftbus::Container *container = server_connection.get_container();
	saftlib::SAFTd saftd(container); // if a non-nullptr is passed to saftd constructor, it will visibly attach timingreceivers when AttachDevice is called
	std::unique_ptr<saftlib::SAFTd_Service> saftd_service(new saftlib::SAFTd_Service(&saftd));
	container->create_object("/de/gsi/saftlib", std::move(saftd_service));
	// end of saftd part

	//saftlib::SAFTd saftd; // without arguments the saftd will not install services to any saftbus::Container
	auto tr_obj_path                 = saftd.AttachDevice("tr0", argv[1], 100);
	saftlib::TimingReceiver* tr      = saftd.getTimingReceiver(tr_obj_path);
	auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
	auto softwareActionSink          = tr->getSoftwareActionSink(softwareActionSink_obj_path);
	auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);
	auto sw_condition                = softwareActionSink->getCondition(condition_obj_path);

	sw_condition->setAcceptEarly(true);
	sw_condition->setAcceptLate(true);
	sw_condition->setAcceptDelayed(true);
	sw_condition->setAcceptConflict(true);

	sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));

	for (;;) {
		saftbus::Loop::get_default().iteration(true);	
	}

	return 0;
}