#include <iostream>
#include <iomanip>

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
	sleep(timeout);
	if (*success) return;
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
		std::cerr << " FAILURE! No exception was thrown" << std::endl;
		++number_of_failures;
	} catch (saftbus::Error &e) {
		success = true;
		std::cerr << " SUCCESS! Got saftbus::Error " << e.what() << std::endl;
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

int main(int argc, char** argv) {
	// if (argc != 2) {
	// 	std::cerr << "ERROR! need etherbone device as command line argument to run the tests" << std::endl;
	// 	std::cerr << "usage: " << argv[0] << " <eb-device>" << std::endl;
	// 	return 1;
	// }


	system("saft-software-tr&");
	system("saftd&");
	sleep(1);
	system("saftbus-ctl --config-log-buffer 99999 3 10 /tmp/saftbus.log");
	std::ifstream eb_device_name_in("/tmp/simbridge-eb-device");
	if (!eb_device_name_in) {
		std::cerr << "ERROR! cannot start software timing receiver" << std::endl;
		return 1;
	}

	std::string eb_device_name;
	eb_device_name_in >> eb_device_name;
	if (!eb_device_name_in) {
		std::cerr << "ERROR! cannot read eb device name from file" << std::endl;
		return 1;
	}

	int number_of_failures = 0;
	std::cout << "Starting saftlib tests" << std::endl;

	number_of_failures += test_exceptions();
	number_of_failures += test_attach_device(eb_device_name.c_str());
	number_of_failures += test_get_property(eb_device_name.c_str());


	if (number_of_failures == 0) {
		std::cerr << "ALL TESTS SUCCESSFUL! " << std::endl;
	}

	system("killall -9 saftd");
	system("killall saft-software-tr");

	return number_of_failures;
}