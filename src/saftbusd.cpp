#include "loop.hpp"
#include "server.hpp"
#include "chunck_allocator_rt.hpp"

int main() {

	saftbus::ServerConnection server_connection;
	saftbus::Loop::get_default().run();
	// in case of sources loaded into the loop, they must be destroyed before the plugins are unloaded
	saftbus::Loop::get_default().clear(); 

	std::cerr << "===================== saftbusd quit ============================" << std::endl;
	return 0;
}