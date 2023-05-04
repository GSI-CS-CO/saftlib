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
	std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 1);

	auto saftd = saftlib::SAFTd_Proxy::create();
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


struct ConditionContainer {
	std::shared_ptr<saftlib::SoftwareCondition_Proxy> condition;
	ConditionContainer(std::shared_ptr<saftlib::SoftwareCondition_Proxy> con) : condition(con)	{
		condition->SigAction.connect(sigc::mem_fun(this, &ConditionContainer::on_action));
	}
	~ConditionContainer() {
	}
	void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
		static int count = 0;
	}
};
static int test_software_condition(const std::string &device) {
	int number_of_failures = 0;
	std::cerr << "TEST creation of SoftwareActionSink and many SoftwareCondition" << std::endl;
	
	bool success = false; // the test should set this to true on success
	// std::thread timeout_thread( &timeout_thread_function, &success, __FUNCTION__, 10);

	bool expect_signals = true;
	std::thread signal_handler( [](bool *signals) { 
		while(*signals) saftlib::wait_for_signal(10);
	}, &expect_signals );

	try {
		auto saftd = saftlib::SAFTd_Proxy::create();
		auto tr  = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()["tr0"]);
		auto sas = saftlib::SoftwareActionSink_Proxy::create(tr->NewSoftwareActionSink(""));
		// auto con = saftlib::SoftwareCondition_Proxy::create(sas->NewCondition(true, 0, 0, 0));
		//con->SigAction.connect(sigc::ptr_fun(&on_action));
		std::vector<std::shared_ptr<ConditionContainer> > conditions;
		std::thread injector([](bool* signals, std::shared_ptr<saftlib::TimingReceiver_Proxy> tr){ 
			int i = 0;
			while(*signals) {
				std::cerr << i ;
				usleep(10000);
				tr->InjectEvent(i++,0,tr->CurrentTime());
			}
		}, &expect_signals, tr );

		for (int i = 0; i < 100; ++i) {
			conditions.push_back(std::make_shared<ConditionContainer>( saftlib::SoftwareCondition_Proxy::create(sas->NewCondition(true, 0,0,0)) ));
		}


		usleep(1000000);
		std::cerr << "clear" << std::endl;
		conditions.clear();
		usleep(100000);

		std::cerr << "SUCCESS! Created softwareCondition "<< std::endl;
		success = true;
		expect_signals = false;
		injector.join();

	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! cannot create SoftwareCondition: " << e.what() << std::endl;
		++number_of_failures;
	}
	signal_handler.join();
	// timeout_thread.join();

	return number_of_failures;
}

int main(int argc, char** argv) {


	system("killall -9 saftbusd");
	system("killall -9 saft-software-tr");
	sleep(1);
	system("saft-software-tr&");
	system("saftbusd libsaft-service.so &");
	sleep(1);
	std::string eb_device_name;
	if (argc == 1) {
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
	} else {
		eb_device_name = argv[1];
	}
	int number_of_failures = 0;
	std::cout << "Starting saftlib tests" << std::endl;

	number_of_failures += test_exceptions();
	number_of_failures += test_attach_device(eb_device_name.c_str());
	number_of_failures += test_get_property(eb_device_name.c_str());
	number_of_failures += test_inject_and_receive_event(eb_device_name.c_str());

	number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// number_of_failures += test_software_condition(eb_device_name.c_str());
	// std::thread new_thread(&test_software_condition, eb_device_name.c_str());
	// new_thread.join();

	if (number_of_failures == 0) {
		std::cerr << "ALL TESTS SUCCESSFUL! " << std::endl;
	}

	system("saftbus-ctl --quit");
	system("killall saft-software-tr");
	system("killall -9 saftbusd");

	return number_of_failures;
}