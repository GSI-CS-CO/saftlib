#include "loop.hpp"
#include "server.hpp"
#include "chunck_allocator_rt.hpp"

int main(int argc, char *argv[]) {

	std::vector<std::string> plugins;
	for (int i = 1; i < argc; ++i) {
		plugins.push_back(argv[i]);
	}

	saftbus::ServerConnection server_connection(plugins);
	saftbus::Loop::get_default().run();
	// in case of sources loaded into the loop, they must be destroyed before the plugins are unloaded
	saftbus::Loop::get_default().clear(); 

	std::cerr << "===================== saftbusd quit ============================" << std::endl;
	return 0;
}