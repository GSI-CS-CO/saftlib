#include "loop.hpp"
#include "server.hpp"
#include "chunck_allocator_rt.hpp"

int main(int argc, char *argv[]) {

	std::vector<std::pair<std::string, std::vector<std::string> > > plugins_and_args;
	for (int i = 1; i < argc; ++i) {
		std::string argvi(argv[i]);
		bool argvi_is_plugin = (argvi.find(".la") == argvi.size()-3);
		if (argvi_is_plugin) {
			std::cerr << argvi << "is plugin name" << std::endl;
			plugins_and_args.push_back(std::make_pair(argvi, std::vector<std::string>()));
		} else {
			std::cerr << argvi << "is argument" << std::endl;
			if (plugins_and_args.empty()) {
				std::cerr << "no plugin specified (these are files ending with .la)" << std::endl;
				return 1;
			} else {
				plugins_and_args.back().second.push_back(argvi);
			}
		}
	}

	saftbus::ServerConnection server_connection(plugins_and_args);
	saftbus::Loop::get_default().run();
	// in case of sources loaded into the loop, they must be destroyed before the plugins are unloaded
	saftbus::Loop::get_default().clear(); 

	std::cerr << "===================== saftbusd quit ============================" << std::endl;
	return 0;
}