#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>


#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/iOwned.h"
#include "CommonFunctions.h"

static void timeout_thread_function(bool *success, const char *testname, int timeout) {
	for (int i = 0 ; i < 1000; ++i) {
		usleep(timeout*1000);
		if (*success) return;
	}
	std::cerr << "FAILURE! Test \"" << testname << "\" exeeded timeout of " << timeout << " sec!" << std::endl;
	exit(1);
}

static int test_exceptions() {
	int number_of_failures = 0;
	std::cerr << "TEST EXCEPTIONS:" << std::endl;
	// start a second thread which will terminate the programm with an error if the test takes too long (=> timout)
	bool success = false;
	std::shared_ptr<saftlib::SAFTd_Proxy> saftd;
	try {
		saftd = saftlib::SAFTd_Proxy::create();
		std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 1);
		try {
			// try to attach a device with an invalid logic name (containing specia characters). 
			// this should trigger an exception
			std::cerr << "trigger an exception from SAFTd... ";
			saftd->AttachDevice("/bad/name/!","dev/nonexistent");
			// if this function call works, there was no exception thrown, which is a failure in this case
			std::cerr << "FAILURE! No exception was thrown" << std::endl;
			++number_of_failures;
		} catch (saftbus::Error &e) {
			success = true;
			std::cerr << "SUCCESS! Got saftbus::Error " << e.what() << std::endl;
		}
		timeout_thread.join();
	} catch (saftbus::Error &e) {
		std::cerr << "FAILURE! SAFTd_Proxy cannot be created: " << e.what() << std::endl;
		++number_of_failures;
	}

	return number_of_failures;
}

static int test_attach_device(const std::string &device) {
	int number_of_failures = 0;
	std::cerr << "TEST ATTACH DEVICE:" << std::endl;
	// start a second thread which will terminate the programm with an error if the test takes too long (=> timout)
	bool success = false;
	std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 1);

	auto saftd = saftlib::SAFTd_Proxy::create();
	try {
		std::string logic_name = "tr0";
		saftd->AttachDevice(logic_name, device);
		auto devices = saftd->getDevices();
		if (devices.find(logic_name) != devices.end()) {
			std::cerr << "SUCCESS! Attached Device tr0 under object_path " << devices["tr0"] << std::endl;
			try { // attach the same device again, this should produce an exception
				saftd->AttachDevice(logic_name, device); 
				std::cerr << "ERROR! Attaching the same device twice should not work" << std::endl;
				++number_of_failures;
			} catch (saftbus::Error &e) {
				std::cerr << "SUCCESS! Attaching the same device twice throws: " << e.what() << std::endl;
				success = true;
			}
		} else {
			std::cerr << "ERRROR! Device not attached under the expected name. Attached devices are: ";
			for (auto device: devices) {
				std::cerr << device.first << ":" << device.second << " ";
			}
			std::cerr << std::endl;
			++number_of_failures;
		}
	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! AttachDevice threw an error: " << e.what() << std::endl;
		++number_of_failures;
	}

	timeout_thread.join();
	return number_of_failures;
}

static int test_get_property(const std::string &device) {
	int number_of_failures = 0;
	std::cerr << "TEST GET PROERTY" << std::endl;
	// start a second thread which will terminate the programm with an error if the test takes too long (=> timout)
	bool success = false;
	std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 1);

	try {
		auto saftd = saftlib::SAFTd_Proxy::create();
		auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()["tr0"]);
		if (device == tr->getEtherbonePath()) {
			std::cerr << "SUCCESS! GetProperty returned the expected EtherbonePath: " << device << std::endl;
			success = true;
		}
	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! GetProperty test threw an error: " << e.what() << std::endl;
		++number_of_failures;
	}


	timeout_thread.join();
	return number_of_failures;
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//           TEST SOFTWARE CONDITIONS
//////////////////////////////////////////////////
//////////////////////////////////////////////////
uint64_t event_id = 0;
uint64_t event_param = 0;
bool action_received = false;
void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
	event_id    = event;
	event_param = param;
	action_received = true;
}
static int test_inject_and_receive_event(const std::string &device) {
	int number_of_failures = 0;
	std::cerr << "TEST injecting event and expect Action signal from SoftwareCondition" << std::endl;
	
	bool success = false; // the test should set this to true on success
	std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 5);

	try {
		auto saftd = saftlib::SAFTd_Proxy::create();
		auto tr  = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()["tr0"]);
		auto sas = saftlib::SoftwareActionSink_Proxy::create(tr->NewSoftwareActionSink(""));
		uint64_t id  = rand();
		uint64_t par = rand();
		auto con = saftlib::SoftwareCondition_Proxy::create(sas->NewCondition(true, id, 0xffffffffffffffff, 0));
		con->SigAction.connect(sigc::ptr_fun(&on_action));
		con->setAcceptLate(true);
		// std::cerr << "inject event and wait for response" << std::endl;
		tr->InjectEvent(id, par, tr->CurrentTime());
		while (!action_received) {
			saftlib::wait_for_signal(1);
		}
		if (par != event_param) throw std::runtime_error("wrong event parameter");
		if (id  != event_id   ) throw std::runtime_error("wrong event id");
		success = true;
		std::cerr << "SUCCESS! Created softwareCondition and received correct event id and param "<< std::endl;
	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! SoftwareCondition: " << e.what() << std::endl;
		++number_of_failures;
	}
	timeout_thread.join();

	return number_of_failures;
}


int action_count = 0;
int signal_count = 0;
struct ConditionContainer {
	std::shared_ptr<saftlib::SoftwareCondition_Proxy> condition;
	ConditionContainer(std::shared_ptr<saftlib::SoftwareCondition_Proxy> con) : condition(con)	{
		condition->SigAction.connect(sigc::mem_fun(this, &ConditionContainer::on_action));
		condition->setAcceptLate(true);
	}
	~ConditionContainer() {
	}
	void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
		++action_count;
		// std::cerr << "on_action " << event << std::endl;
	}
};
static int test_software_condition_with_treads(const std::string &device) {
	int number_of_failures = 0;
	std::cerr << "TEST creation of SoftwareActionSink and many SoftwareConditions with separate threads" << std::endl;
	
	bool success = false; // the test should set this to true on success

	// this thread terminates the test after 30 seconds (if the success variable is not set to true at the end of the test)
	std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 30);

	std::mutex signal_mutex;
	bool expect_signals = true;

	// start a thread that continually waits for signals until the *signals viable is set to false somewhere outside (to indicate the end of the test)
	std::thread signal_handler( [](bool *signals, std::mutex *m) { 
		while(*signals) {
			std::lock_guard<std::mutex> lock(*m);
			saftlib::wait_for_signal(1);
		}
	}, &expect_signals, &signal_mutex );

	try {
		auto saftd = saftlib::SAFTd_Proxy::create();
		auto tr  = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()["tr0"]);
		auto sas = saftlib::SoftwareActionSink_Proxy::create(tr->NewSoftwareActionSink(""));
		std::vector<std::shared_ptr<ConditionContainer> > conditions;

		// start a thread that injects events with 100 Hz (1 event every 10 ms)
		std::thread injector([](bool* signals, std::shared_ptr<saftlib::TimingReceiver_Proxy> tr){ 
			int i = 0;
			while(*signals) {
				usleep(10000);
				tr->InjectEvent(i++,0,tr->CurrentTime());
				++signal_count;
			}
		}, &expect_signals, tr );

		// create some conditions while events are injected
		int N_conditions = 10;
		for (int i = 0; i < N_conditions; ++i) {
			std::lock_guard<std::mutex> lock(signal_mutex);
			conditions.push_back(std::make_shared<ConditionContainer>( saftlib::SoftwareCondition_Proxy::create(sas->NewCondition(true, 0,0,0)) ));
		}

		// wait a little bit (0.1 s) to capture some on_action callbacks ...
		usleep(100000);
		// ... then destroy all conditions
		{
			std::lock_guard<std::mutex> lock(signal_mutex);
			conditions.clear();
		}
		expect_signals = false;
		success = true;
		injector.join();

		if (action_count == 0) {
			std::cerr << "ERROR! on_action callback function was never called (" << signal_count << "signals were sent, " << N_conditions << " were created)" << std::endl;
			++number_of_failures;
		} else {
			std::cerr << "SUCCESS! received " << action_count << " actions (" << signal_count << " signals were sent, " << N_conditions << " were created) " << std::endl;
		}
	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! multithread test failed: " << e.what() << std::endl;
		++number_of_failures;
	}
	signal_handler.join();
	timeout_thread.join();

	return number_of_failures;
}

int main(int argc, char** argv) {
	std::string eb_device_name;
	bool run_server = true;
	for (int i = 1; i < argc; ++i) {
		std::string argvi = argv[i];
		if (argvi == "-s") {
			run_server = false;
		} else if (argvi == "-h" || argvi == "--help") {
			std::cout << "usage: " << argv[0] << " [-s] [-h] [<eb-device-name>]" << std::endl;
			std::cout << "  <eb-device-name> optional, if it is missing, saft-software-tr will be started and used for the tests." << std::endl;
			std::cout << "  -s               don\'t start saftbusd (useful if saftbusd is already running" << std::endl;
			std::cout << "  -h               print this help" << std::endl;
			std::cout << std::endl;
			std::cout << "examples: " << std::endl;
			std::cout << "  saft-testbech                    run the testbench, the program will start an instance of saft-software-tr and " << std::endl;
			std::cout << "                                   saftbusd and attach the simulated hardware" << std::endl;
			std::cout << "  saft-testbech dev/ttyUSB0        run the testbench, the program will start an instance of and saftbusd and " << std::endl;
			std::cout << "                                   attach dev/ttyUSB0" << std::endl;
			std::cout << "  saft-testbech -s dev/ttyUSB0     run the testbench, an instance of saftbusd must already be running, the " << std::endl;
			std::cout << "                                   programm will attach dev/ttyUSB0" << std::endl;
			std::cout << "  saft-testbech -s                 run the testbench, an instance of saftbusd must already be running, the program" << std::endl; 
			std::cout << "                                   will start an instance of saft-software-tr and will attach it to the running saftbusd" << std::endl;
			return 0;
		} else if(argvi.size() > 4 && argvi.substr(0,4) == "dev/") {
			eb_device_name = argvi;
		}
	}

	bool run_software_tr = false;
	if (eb_device_name.size() == 0) {
		run_software_tr = true;
		if (run_software_tr) system("killall -9 saft-software-tr");
		if (run_software_tr) system("saft-software-tr&");
		usleep(10000);
		std::ifstream eb_device_name_in("/tmp/simbridge-eb-device");
		if (!eb_device_name_in) {
			std::cerr << "ERROR! cannot start software timing receiver" << std::endl;
			return 1;
		}
		eb_device_name_in >> eb_device_name;
		if (!eb_device_name_in) {
			std::cerr << "ERROR! cannot read eb device name from file" << std::endl;
			return 1;
		}
	}

	if (run_server)      system("killall -9 saftbusd");
	if (run_server)      system("saftbusd libsaft-service.so &");
	usleep(10000);




	int number_of_failures = 0;
	std::cout << "Starting saftlib tests" << std::endl;

	number_of_failures += test_exceptions();
	number_of_failures += test_attach_device(eb_device_name.c_str());
	number_of_failures += test_get_property(eb_device_name.c_str());
	number_of_failures += test_inject_and_receive_event(eb_device_name.c_str());
	number_of_failures += test_software_condition_with_treads(eb_device_name.c_str());

	if (number_of_failures == 0) {
		std::cerr << "ALL TESTS SUCCESSFUL! " << std::endl;
	}


	if (run_software_tr) system("killall saft-software-tr");
	// if (run_server)      system("saftbus-ctl --quit");
	if (run_server)      system("killall -9 saftbusd");

	return number_of_failures;
}