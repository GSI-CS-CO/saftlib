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
#include <cerrno>
#include <cstring>

std::chrono::time_point<std::chrono::steady_clock> start, stop;
std::vector<int> histogram(10000);

long shutdown_threshold = 0;
bool shutdown = false;

std::ofstream trace_marker;

static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
	if (shutdown_threshold) {
		trace_marker << "saft-standalone-roundtrip-latency MSI callback function executed" << std::endl;
	}

	stop = std::chrono::steady_clock::now();
	std::chrono::nanoseconds duration = stop-start;
	int us = duration.count()/1000;
	if (us >= 0 && us < (int)histogram.size()) {
		histogram[us]++;
	}

	if (shutdown_threshold && us > shutdown_threshold) {
		shutdown = true;
		std::ofstream tracing_on("/sys/kernel/debug/tracing/tracing_on");
		tracing_on << "0\n";
		std::cerr << "time measurement (" << us << " us) was larger than threshold (" << shutdown_threshold << " us ). => writing 0 into /sys/kernel/debug/tracing/tracing_on and exit" << std::endl;
	}

} 

int main(int argc, char *argv[]) {
	if (argc != 4 && argc != 5) {
		std::cerr << "Measure several times the duration from InjectEvent until callback" << std::endl;
		std::cerr << "function and create a histogram of the measurment results" << std::endl;
		std::cerr << "usage: " << argv[0] << " <eb-device> <number-of-measurements> <histogram-filename> [ <trace-shutoff-threshold[us]> ] " << std::endl;
		std::cout << std::endl;
		std::cerr << "   example:                              " << argv[0] << " dev/wbm0 1000 histogram.dat" << std::endl;
		std::cerr << "   example with trace-shutoff-threshold: " << argv[0] << " dev/wbm0 1000 histogram.dat 1000" << std::endl;
		return 1;
	}

	if (argc == 5) { // read shutdown threshold
		std::istringstream s_in(argv[4]);
		s_in >> shutdown_threshold;
		if (!s_in) {
			std::cerr << "cannot read trace-shutoff-threshold from argument " << argv[5] << std::endl;
			return 1;
		}
		std::cerr << "running with trace-shutoff-threshold of " << shutdown_threshold << " us" << std::endl;
	}

	if (shutdown_threshold) { // open trace marker file
		trace_marker.open("/sys/kernel/tracing/trace_marker");
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
			if (i && (i%10000)==0) {
				std::ofstream hist("hist.tmp");
				hist << "# time[us] counts" << std::endl;
				for (unsigned i = 0 ; i < histogram.size() ; ++i) {
					hist << i << " " << histogram[i] << std::endl;
				}
				hist.close();
				std::string cmd("mv hist.tmp ");
				cmd.append(argv[3]);
				system(cmd.c_str());
				std::cerr << "write histogram " << argv[3] << " " << 10000*(i/10000) << " measurements" << std::endl;
			}
			start = std::chrono::steady_clock::now();
			if (shutdown_threshold) {
				trace_marker << "saft-standalone-roundtrip-latency InjectEvent" << std::endl;
			}
			tr->InjectEvent(0xaffe, 0x0, saftlib::makeTimeTAI(0));
			if (shutdown_threshold) {
				trace_marker << "saft-standalone-roundtrip-latency iterate MainLoop" << std::endl;
			}
			saftbus::Loop::get_default().iteration(true);
			if (shutdown) break;
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
