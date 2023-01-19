#include "SAFTd_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SoftwareActionSink_Proxy.hpp"
#include "SoftwareCondition_Proxy.hpp"
#include "CommonFunctions.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <exception>
#include <chrono>
#include <map>

std::chrono::time_point<std::chrono::steady_clock> start, stop;
std::vector<int> histogram(10000);


static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
	stop = std::chrono::steady_clock::now();
	std::chrono::nanoseconds duration = stop-start;
	int us = duration.count()/1000;
	if (us >= 0 && us < (int)histogram.size()) {
		histogram[us]++;
	}
} 

int main(int argc, char *argv[]) {
	if (argc != 4) {
		std::cerr << "usage: " << argv[0] << " <eb-device> <number-of-measurements> <histogram-filename>" << std::endl;
		return 1;
	}
	try {
		auto saftd = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib");
		auto tr    = saftlib::TimingReceiver_Proxy::create(std::string("/de/gsi/saftlib/")+argv[1]);

		auto software_action_sink_object_path = tr->NewSoftwareActionSink("");
		auto software_action_sink             = saftlib::SoftwareActionSink_Proxy::create(software_action_sink_object_path);
		auto condition_object_path            = software_action_sink->NewCondition(true, 0xaffe,-1,0);
		auto condition                        = saftlib::SoftwareCondition_Proxy::create(condition_object_path);
		condition->setAcceptEarly(true);
		condition->setAcceptLate(true);
		condition->setAcceptConflict(true);
		condition->setAcceptDelayed(true);
		condition->SigAction.connect(sigc::ptr_fun(&on_action));

		int N;
		std::istringstream Nin(argv[2]);
		Nin >> N;
		if (!Nin) {
			std::cerr << "cannot read number-of-measurements from " << argv[2] << std::endl;
			return 1;
		}

		for (int i = 0; i < N; ++i) {
			start = std::chrono::steady_clock::now();
			tr->InjectEvent(0xaffe, 0x0, saftlib::makeTimeTAI(0));
			saftlib::wait_for_signal();
		}

		std::ofstream hist(argv[3]);
		hist << "# time[us] counts" << std::endl;
		for (unsigned i = 0 ; i < histogram.size() ; ++i) {
			hist << i << " " << histogram[i] << std::endl;
		}

	} catch (std::runtime_error &e ) {
		std::cerr << "exception: " << e.what() << std::endl;
	}

	return 0;
}
