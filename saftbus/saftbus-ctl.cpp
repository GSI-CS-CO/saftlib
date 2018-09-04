#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
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

void show_help(const char *argv0) {
	std::cerr << "usage: " << argv0 << "[options]" << std::endl;
	std::cerr << "   options are:" << std::endl;
	std::cerr << "   -l     list all active devices in saftlib (object paths)" << std::endl;
	std::cerr << "   -p     show all open signal pipes to proxy objects" << std::endl;
	std::cerr << "   -s     write timing statistics to file" << std::endl;
}

int main(int argc, char *argv[])
{

	Glib::init();

	bool list_objects                 = false;
	bool list_pipes                   = false;
	bool get_timing_stats             = false;
	std::string timing_stats_filename = "saftbus_timing.dat";

	for (int i = 1; i < argc; ++i) {
		std::string argvi = argv[i];
		std::cerr << argvi << std::endl;

		if (argvi == "-l") {
			list_objects = true;
		} else if (argvi == "-p") {
			list_pipes = true;
		} else if (argvi == "-s") {
			get_timing_stats = true;
			// if (++i < argc) {
			// 	timing_stats_filename = argv[i];
			// 	std::cerr << timing_stats_filename << std::endl;
			// }
		} else {
			std::cerr << "unknown argument: " << argvi << std::endl;
			return 1;
		}
	}

	// connect to saft-daemon
	auto connection = Glib::RefPtr<saftbus::ProxyConnection>(new saftbus::ProxyConnection);

	// say hello to saftbus
	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_HELLO);

	// tell saftbus to send us some information
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


	if (list_objects) {
		std::cerr << "listing all objects: " << std::endl;
		for( auto it: object_paths) {
			std::cerr << "    " << it << std::endl;
		}
	}

	if (list_pipes) {
		std::cerr << "listing all active indices: " << std::endl;
		for (auto it: indices) {
			std::cerr << "    " << it << std::endl;
		}
	}

	if (get_timing_stats) {
		write_histogram(timing_stats_filename.c_str(), signal_flight_times);
		for (auto function: function_run_times) {
			write_histogram(function.first, function.second);
		}
	}


	connection.reset();
	std::cerr << "saftbus-ctl done" << std::endl;
	return 0;
}