#include <iostream>
#include <iomanip>
#include "saftbus.h"
#include "core.h"


int main()
{
	auto connection = Glib::RefPtr<saftbus::ProxyConnection>(new saftbus::ProxyConnection);


	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_HELLO);

	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_STATUS);
	std::map<Glib::ustring, std::map<Glib::ustring, int> > saftbus_indices;
	saftbus::read(connection->get_fd(), saftbus_indices);

	std::set<Glib::ustring> object_paths;

	for (auto itr: saftbus_indices) {
		for (auto it: itr.second) {
			object_paths.insert(it.first);
		}
	}

	std::cerr << "listing all objects: " << std::endl;
	for( auto it: object_paths) {
		std::cerr << "    " << it << std::endl;
	}



	connection.reset();
	std::cerr << "saftbus-ctl done" << std::endl;
	return 0;
}