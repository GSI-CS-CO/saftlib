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
#include <cerrno>
#include <cstring>

#include <sched.h>


std::chrono::time_point<std::chrono::steady_clock> start, stop;
std::vector<int> histogram(10000);


static bool set_realtime_scheduling(std::string argvi, char *prio) {
	std::istringstream in(prio);
	sched_param sp;
	in >> sp.sched_priority;
	if (!in) {
		std::cerr << "Error: cannot read priority from argument " << prio << std::endl;
		return false;
	}
	int policy = SCHED_RR;
	if (argvi == "-f") policy = SCHED_FIFO;
	if (sp.sched_priority < sched_get_priority_min(policy) && sp.sched_priority > sched_get_priority_max(policy)) {
		std::cerr << "Error: priority " << sp.sched_priority << " not supported " << std::endl;
		return false;
	} 
	if (sched_setscheduler(0, policy, &sp) < 0) {
		std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

static bool set_cpu_affinity(std::string argvi, char *affinity) {
	// affinity should be a comma-separated list such as "1,4,5,9"
	cpu_set_t set;
	CPU_ZERO(&set);
	std::string affinity_list = affinity;
	for(auto &ch: affinity_list) if (ch==',') ch=' ';
	std::istringstream in(affinity_list);
	int count = 0;
	for (;;) {
		int CPU;
		in >> CPU;
		if (!in) break;
		CPU_SET(CPU, &set);
		++count;
	}
	if (!count) {
		std::cerr << "Error: cannot read cpus from argument " << affinity << std::endl;
		return false;
	}
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
		std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}


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
	if (argc < 4) {
		std::cerr << "Measure several times the duration from InjectEvent until callback" << std::endl;
		std::cerr << "function and create a histogram of the measurment results" << std::endl;
		std::cerr << "usage: " << argv[0] << " <saftlib-device> <number-of-measurements> <histogram-filename> [OPTIONS] " << std::endl;
		std::cout << "options: " << std::endl;
		std::cout << std::endl;
		std::cout << " -r <priority>    set scheduling policy to round robin with given priority." << std::endl;
		std::cout << "                  priority must be in the range [" << sched_get_priority_min(SCHED_RR) << " .. " << sched_get_priority_max(SCHED_RR) << "]" << std::endl;
		std::cout << std::endl;
		std::cout << " -f <priority>    set scheduling policy to fifo with given priority." << std::endl;
		std::cout << "                  priority must be in the range [" << sched_get_priority_min(SCHED_FIFO) << " .. " << sched_get_priority_max(SCHED_FIFO) << "]" << std::endl;
		std::cout << std::endl;
		std::cout << " -a <cpu>         set affinity of this process to cpu." << std::endl;
		std::cout << std::endl;
		std::cerr << "   example: " << argv[0] << " tr0 1000 histogram.dat" << std::endl;
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

		for (int i = 4; i < argc; ++i) {
			std::string argvi(argv[i]);
			if (argvi == "-r" || argvi == "-f" || argvi == "-a") {
				if (++i < argc) {
					if (argvi == "-a") {
						if (!set_cpu_affinity(argvi, argv[i])) return 1;
					} else {
						if (!set_realtime_scheduling(argvi, argv[i])) return 1;
					}
				} else {
					std::cerr << "Error: expect priority after " << argvi << std::endl;
					return 1;
				}
			}
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
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}
