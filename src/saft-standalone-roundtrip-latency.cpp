#include "SAFTd.hpp"
#include "TimingReceiver.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareCondition.hpp"
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
		std::cerr << "Measure several times the duration from InjectEvent until callback" << std::endl;
		std::cerr << "function and create a histogram of the measurment results" << std::endl;
		std::cerr << "usage: " << argv[0] << " <eb-device> <number-of-measurements> <histogram-filename>" << std::endl;
		std::cerr << "   example: " << argv[0] << " dev/wbm0 1000 histogram.dat" << std::endl;
		return 1;
	}
	try {
		auto saftd = std::make_shared<saftlib::SAFTd>();
		auto tr    = std::make_shared<saftlib::TimingReceiver>(*saftd, "tr0", argv[1]);

		auto software_action_sink_object_path = tr->NewSoftwareActionSink("");
		auto software_action_sink             = tr->getSoftwareActionSink(software_action_sink_object_path);
		auto condition_object_path            = software_action_sink->NewCondition(true, 0xaffe,-1,0);
		auto condition                        = software_action_sink->getCondition(condition_object_path);
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
			saftbus::Loop::get_default().iteration(true);
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
