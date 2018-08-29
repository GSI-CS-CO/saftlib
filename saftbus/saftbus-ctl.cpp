#include <iostream>
#include <fstream>
#include <iomanip>
#include "saftbus.h"
#include "core.h"
#include "Interface.h"


int main()
{
	Glib::init();
	auto connection = Glib::RefPtr<saftbus::ProxyConnection>(new saftbus::ProxyConnection);


	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_HELLO);

	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_STATUS);


	std::map<Glib::ustring, std::map<Glib::ustring, int> > saftbus_indices;
	saftbus::read(connection->get_fd(), saftbus_indices);

	std::vector<int> indices;
	saftbus::read(connection->get_fd(), indices);

	std::map<int, int> signal_flight_times;
	saftbus::read(connection->get_fd(), signal_flight_times);

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

	std::cerr << "listing all active indices: " << std::endl;
	for (auto it: indices) {
		std::cerr << "    " << it << std::endl;
	}

	std::ofstream signal_flight_times_out("signal_flight_times.dat");
	for (auto it: signal_flight_times) {
		std::cerr << it.first << " " << it.second << std::endl;
		signal_flight_times_out << it.first << " " << it.second << std::endl;
	}



	connection.reset();
	std::cerr << "saftbus-ctl done" << std::endl;
	return 0;
}