#include <saftbus.hpp>
#include <loop.hpp>
#include "eb-source.hpp"

int main() {

	etherbone::Socket socket;
	socket.open();
	// saftbus::Loop::get_default().connect<eb_plugin::EB_Source>(socket);
	for (;;) {
		// saftbus::Loop::get_default().iteration(true);
	}

	return 0;
}