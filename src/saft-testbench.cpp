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

static int test_exceptions() {
	int number_of_failures = 0;
	std::cerr << "TEST EXCEPTIONS:" << std::endl;
	auto saftd = saftlib::SAFTd_Proxy::create();

	// start a second thread which will terminate the programm with an error if the test takes too long (=> timout)
	bool success = false;
	std::thread timeout_thread( [](bool *success){
			sleep(1);
			if (*success) return;
			std::cerr << "FAILURE! Test timeout 1 sec. No exception arrived!" << std::endl;
			exit(1);
		}, &success);
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
	auto saftd = saftlib::SAFTd_Proxy::create();
	try {
		saftd->AttachDevice("tr0", device);
	} catch (saftbus::Error &e) {
		std::cerr << "ERRROR! AttachDevice threw an error: " << e.what() << std::endl;
		++number_of_failures;
	}
	return number_of_failures;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "ERROR! need etherbone device as command line argument to run the tests" << std::endl;
		std::cerr << "usage: " << argv[0] << " <eb-device>" << std::endl;
		return 1;
	}

	int number_of_failures = 0;
	std::cout << "Starting saftlib tests" << std::endl;

	number_of_failures += test_exceptions();
	number_of_failures += test_attach_device(argv[1]);

	if (number_of_failures == 0) {
		std::cerr << "ALL TESTS SUCCESSFUL! " << std::endl;
	}

	return number_of_failures;
}