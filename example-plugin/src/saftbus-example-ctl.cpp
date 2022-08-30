#include "Dice2_Proxy.hpp"
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

	auto dice = example::Dice2_Proxy::create("/example");

	// if (setmax >= 1) {
	// 	std::cerr << "old max: " << dice->getMax() << std::endl;
	// 	std::cerr << "set max to " << setmax << std::endl;
	// 	dice->setMax(setmax);
	// }
	std::cerr << "dice roll: " << dice->roll2() << std::endl;
	return 0;
}