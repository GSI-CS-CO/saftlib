#include "Dice_Proxy.hpp"
#include <saftbus/client.hpp>
#include <sigc++/sigc++.h> 
#include <functional> 
#include <string>

std::string cmd, object_path;
void print_result_sig(int result) {
	std::cout << "dice " << object_path << " was thrown. SigResult = " << result << std::endl;
}
void print_result_fun(int result) {
	std::cout << "dice " << object_path << " was thrown. FunResult = " << result << std::endl;
}
int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cout << "usage: " << argv[0] << " <object-path> throw|listen" << std::endl;
		return 1;
	}
	object_path = argv[1];
	cmd         = argv[2];

	auto dice = ex00::Dice_Proxy::create(object_path);

	dice->SigResult.connect(sigc::ptr_fun(print_result_sig));
	dice->FunResult = &print_result_fun;
	if (cmd == "throw") {
		dice->Throw();
	}
	for (;;) {
		saftbus::SignalGroup::get_global().wait_for_signal();
		if (cmd != "listen") break;
	}
	return 0;
}