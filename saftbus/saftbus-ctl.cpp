#include <iostream>
#include <fstream>
#include <iomanip>
#include "saftbus.h"
#include "core.h"
#include "Interface.h"


void write_histogram(Glib::ustring filename, const std::map<int,int> &hist)
{
	std::ofstream out(filename.c_str());
	for (auto it: hist) {
		out << it.first << " " << it.second << std::endl;
	}

}

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

	std::map<Glib::ustring, std::map<int, int> > function_run_times;
	saftbus::read(connection->get_fd(), function_run_times);


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

	write_histogram("signal_flight_times.dat", signal_flight_times);
	for (auto function: function_run_times) {
		write_histogram(function.first, function.second);
	}


	connection.reset();
	std::cerr << "saftbus-ctl done" << std::endl;
	return 0;
}