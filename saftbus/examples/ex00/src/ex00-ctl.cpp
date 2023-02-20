#include "Dice_Proxy.hpp"
#include <saftbus/client.hpp>
#include <sigc++/sigc++.h>
#include <string>

std::string cmd, object_path;
void print_result(int result) {
	std::cout << "dice " << object_path << " was thrown. Result = " << result << std::endl;
}
int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cout << "usage: " << argv[0] << " <object-path> throw|listen" << std::endl;
		return 1;
	}
	object_path = argv[1];
	cmd         = argv[2];

	auto dice = ex00::Dice_Proxy::create(object_path);

	dice->Result.connect(sigc::ptr_fun(print_result));
	if (cmd == "throw") {
		dice->Throw();
	}
	for (;;) {
		saftbus::SignalGroup::get_global().wait_for_signal();
		if (cmd != "listen") break;
	}
	return 0;
}