#include "SpecialDice_Proxy.hpp"
#include <saftbus/client.hpp>
// #include <saftbus/make_unique.hpp>

void signal1(int a,int b, int c) {
	std::cerr << "+++++++++++++++ signal1(" << a << "," << b << "," << c << ")" << std::endl;
}

int main(int argc, char **argv) {
	// int setmax = 100;
	// if (argc == 2) {
	// 	std::istringstream maxin(argv[1]);
	// 	maxin >> setmax;
	// 	if (!maxin) {
	// 		std::cerr << "cannot read max value from " << argv[1] << std::endl;
	// 		return 1;
	// 	}
	// }

	auto dice = example::SpecialDice_Proxy::create("/example");
	dice->signal1 = &signal1;


	// if (setmax >= 1) {
	// 	std::cerr << "old max: " << dice->getMax2() << std::endl;
	// 	std::cerr << "set max to " << setmax << std::endl;
	// 	dice->setMax2(setmax);
	// }
	auto start = std::chrono::steady_clock::now();
	auto stop = std::chrono::steady_clock::now();

	start = std::chrono::steady_clock::now();
	int roll = dice->roll();
	stop = std::chrono::steady_clock::now();
	std::cerr << "roll()=" << roll << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

	// start = std::chrono::steady_clock::now();
	// int roll2 = dice->roll2();
	// stop = std::chrono::steady_clock::now();
	// std::cerr << "roll2()=" << roll2 << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

	// start = std::chrono::steady_clock::now();
	// double rollf = dice->rollf();
	// stop = std::chrono::steady_clock::now();
	// std::cerr << "rollf()=" << rollf << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

	// dice->trigger_exception();


	auto core_service_proxy = saftbus::Container_Proxy::create();
	saftbus::SaftbusInfo saftbus_info = core_service_proxy->get_status();


	dice->getMax();
	saftbus::SignalGroup::get_global().wait_for_signal();

	return 0;
}