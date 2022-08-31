#include "SpecialDice_Proxy.hpp"
// #include <saftbus/make_unique.hpp>

int main(int argc, char **argv) {
	// int setmax = -1;
	// if (argc == 2) {
	// 	std::istringstream maxin(argv[1]);
	// 	maxin >> setmax;
	// 	if (!maxin) {
	// 		std::cerr << "cannot read max value from " << argv[1] << std::endl;
	// 		return 1;
	// 	}
	// }

	auto dice = example::SpecialDice_Proxy::create("/example");

	// if (setmax >= 1) {
	// 	std::cerr << "old max: " << dice->getMax() << std::endl;
	// 	std::cerr << "set max to " << setmax << std::endl;
	// 	dice->setMax(setmax);
	// }
	auto start = std::chrono::steady_clock::now();
	auto stop = std::chrono::steady_clock::now();

	start = std::chrono::steady_clock::now();
	int roll = dice->roll();
	stop = std::chrono::steady_clock::now();
	std::cerr << "roll()=" << roll << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

	start = std::chrono::steady_clock::now();
	int roll2 = dice->roll2();
	stop = std::chrono::steady_clock::now();
	std::cerr << "roll2()=" << roll2 << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;

	start = std::chrono::steady_clock::now();
	double rollf = dice->rollf();
	stop = std::chrono::steady_clock::now();
	std::cerr << "rollf()=" << rollf << "   took " << std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count() << " us" << std::endl;


	return 0;
}