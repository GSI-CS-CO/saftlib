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

int main() {
	std::cout << "starting saftlib tests" << std::endl;

	int number_of_failures = 0;

	number_of_failures += test_exceptions();

	if (number_of_failures == 0) {
		std::cerr << "ALL TESTS SUCCSEDED! " << std::endl;
	}

	return number_of_failures;
}