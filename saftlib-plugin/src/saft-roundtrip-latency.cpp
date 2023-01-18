#include "SAFTd_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SoftwareActionSink_Proxy.hpp"
#include "SoftwareCondition_Proxy.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <exception>
#include <chrono>
#include <map>

std::chrono::time_point<std::chrono::steady_clock> start, stop;

std::vector<int> histogram(10000);

std::shared_ptr<saftlib::SAFTd_Proxy>          saftd;
std::shared_ptr<saftlib::TimingReceiver_Proxy> tr;


static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
	stop = std::chrono::steady_clock::now();
	std::chrono::nanoseconds duration = stop-start;
	// std::cerr << "on_action " << duration.count() << std::endl;
	int us = duration.count()/1000;
	if (us >= 0 && us < (int)histogram.size()) {
		histogram[us]++;
	}
} 

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <saftlib-devic>" << std::endl;
		return 1;
	}
	try {
		saftd = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib");
		tr    = saftlib::TimingReceiver_Proxy::create("/de/gsi/saftlib/tr0");

		auto software_action_sink_object_path = tr->NewSoftwareActionSink("");
		auto software_action_sink             = saftlib::SoftwareActionSink_Proxy::create(software_action_sink_object_path);
		auto condition_object_path            = software_action_sink->NewCondition(true, 0xaffe,-1,0);
		auto condition                        = saftlib::SoftwareCondition_Proxy::create(condition_object_path);
		condition->setAcceptEarly(true);
		condition->setAcceptLate(true);
		condition->setAcceptConflict(true);
		condition->setAcceptDelayed(true);
		condition->SigAction.connect(sigc::ptr_fun(&on_action));

		for (int i = 0; i < 10000; ++i) {
			// std::cerr << "blub" << std::endl;
			auto now = tr->CurrentTime();
			start = std::chrono::steady_clock::now();
			tr->InjectEvent(0xaffe, 0x0, now);
			// saftbus::Loop::get_default().iteration(true);
			saftbus::SignalGroup::get_global().wait_for_signal();
		}

		std::ofstream hist("histogram_saftbus.dat");
		for (unsigned i = 0 ; i < histogram.size() ; ++i) {
			hist << i << " " << histogram[i] << std::endl;
		}

	} catch (std::runtime_error &e ) {
		std::cerr << "exception: " << e.what() << std::endl;
	}

	return 0;
}
