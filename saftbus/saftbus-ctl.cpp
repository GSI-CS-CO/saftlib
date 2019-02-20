#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include "saftbus.h"
#include "core.h"
#include "Interface.h"

void write_histogram(std::string filename, const std::map<int,int> &hist);
void show_help(const char *argv0);
void print_mutable_state(std::shared_ptr<saftbus::ProxyConnection> connection);

void write_histogram(std::string filename, const std::map<int,int> &hist)
{
	std::cout << "writing histogram " << filename << std::endl;
	std::ofstream out(filename.c_str());
	for (auto it: hist) {
		out << it.first << " " << it.second << std::endl;
	}

}

void show_help(const char *argv0) 
{
	std::cout << "usage: " << argv0 << " [options]" << std::endl;
	std::cout << "   options are:" << std::endl;
	std::cout << "   -s     print status" << std::endl;
	std::cout << "   -t     write signal flight time statistics to file" << std::endl;
	std::cout << "   -e     enable signal flight time statistics" << std::endl;
	std::cout << "   -d     disable signal flight time statistics" << std::endl;
	std::cout << "   -h     show this help" << std::endl;
}


void print_mutable_state(std::shared_ptr<saftbus::ProxyConnection> connection) 
{
	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_GET_STATE);


	std::map<std::string, std::map<std::string, int> > saftbus_indices; 
	saftbus::read(connection->get_fd(), saftbus_indices);
	std::vector<int> indices;
	std::vector<int> assigned_indices;
	saftbus::read(connection->get_fd(), indices);

	int saftbus_object_id_counter; // log saftbus object creation
	saftbus::read(connection->get_fd(), saftbus_object_id_counter);
	int saftbus_signal_handle_counter; // log signal subscriptions
	saftbus::read(connection->get_fd(), saftbus_signal_handle_counter);

	std::vector<int> sockets_active;
	saftbus::read(connection->get_fd(), sockets_active);

	std::map<int, std::string> socket_owner;
	saftbus::read(connection->get_fd(), socket_owner);

	// 	     // handle    // signal
	//std::map<unsigned, sigc::signal<void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Glib::VariantContainerBase&> > _handle_to_signal_map;
	std::map<unsigned, int> handle_to_signal_map;
	saftbus::read(connection->get_fd(), handle_to_signal_map);
	// for (auto handle_signal: handle_to_signal_map) {
	// 	std::cout << handle_signal.first << " " << handle_signal.second << std::endl;
	// }


	std::map<std::string, std::set<unsigned> > id_handles_map;
	saftbus::read(connection->get_fd(), id_handles_map);


	//std::set<unsigned> erased_handles;
	std::vector<unsigned> erased_handles;
	saftbus::read(connection->get_fd(), erased_handles);



	// store the pipes that go directly to one or many Proxy objects
			// interface_name        // object path
	std::map<std::string, std::map < std::string , std::set< saftbus::ProxyPipe > > > proxy_pipes;
	saftbus::read(connection->get_fd(), proxy_pipes);

	int _saftbus_id_counter;
	saftbus::read(connection->get_fd(), _saftbus_id_counter);

	std::map<std::string, std::string> owners;
	saftbus::read(connection->get_fd(), owners);



	std::cout << "_____________________________________________________________________________________________________________" << std::endl;
	std::cout << std::endl;
	std::cout << "object id counter:     " << saftbus_object_id_counter << std::endl;
	std::cout << "signal handle counter: " << saftbus_signal_handle_counter << std::endl;
	std::cout << "_____________________________________________________________________________________________________________" << std::endl;
	std::cout << std::endl;
	std::cout << "socket: ";
	for(unsigned i = 0; i < sockets_active.size(); ++i ) {
		std::cout << std::setw(3) << i;
	}
	std::cout << std::endl;
	std::cout << "busy:   ";
	for(unsigned i = 0; i < sockets_active.size(); ++i ) {
		if (sockets_active[i]) {
			std::cout << std::setw(3) << "*";
		} else {
			std::cout << std::setw(3) << " ";
		}
	}
	std::cout << std::endl;
	std::cout << "_____________________________________________________________________________________________________________" << std::endl;
	std::cout << std::endl;

	std::cout << std::left << std::setw(40) << "interface name" 
	          << std::left << std::setw(50) << "object path \"owner\"" 
	          << std::right << std::setw(10) << "index" 
	          << "   vtable" << std::endl;
	std::cout << "_____________________________________________________________________________________________________________" << std::endl;
	std::cout << std::endl;
	for (auto saftbus_index: saftbus_indices) {
		std::cout << std::left << std::setw(40) << saftbus_index.first;
		bool first = true;
		for (auto object_path: saftbus_index.second) {
			if (first) {
				std::cout << std::left << std::setw(50) << object_path.first 
				          << std::right << std::setw(10) << object_path.second;
				first = false;
			} else {
				std::string obj_path = object_path.first;
				if (saftbus_index.first == "de.gsi.saftlib.Owned") {
					std::string owner = owners[object_path.first];
					if (owner != "") {
						obj_path.append(" \"");
						obj_path.append(owner);
						obj_path.append("\"");
					}
					//std::cerr << " \"" << owners[object_path.first] << "\"" << std::endl;
				}

				std::cout << std::left << std::setw(40) << " " 
				          << std::left << std::setw(50) << obj_path
				          << std::right << std::setw(10) << object_path.second;
			}
			if (find(indices.begin(), indices.end(), object_path.second) != indices.end()) {
				std::cout << "   yes";
				assigned_indices.push_back(object_path.second);
			}
			// if (saftbus_index.first == "de.gsi.saftlib.Owned") {
			// 	std::cerr << " \"" << owners[object_path.first] << "\"" << std::endl;
			// }

			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << "_____________________________________________________________________________________________________________" << std::endl;


	if (assigned_indices.size() != indices.size()) {
		std::cout << "_____________________________________________________________________________________________________________" << std::endl;
		std::cout << "found vtable indices without saftbus object assigned" << std::endl;
		std::cout << "_____________________________________________________________________________________________________________" << std::endl;
	}
	std::cout << std::endl;
	std::cout << std::setw(7) << "socket" << std::setw(7) << " owner" << std::endl;
	for (auto owner: socket_owner) {
		std::cout << std::setw(7) << owner.first << std::setw(7) << owner.second << std::endl;
	}
	std::cout << std::endl;
	//int _client_id;

}

int main(int argc, char *argv[])
{
	try {
		//Glib::init();

		bool list_mutable_state           = false;
		bool enable_signal_stats          = false;
		bool disable_signal_stats         = false;
		bool save_signal_time_stats       = false;
		std::string timing_stats_filename = "saftbus_timing.dat";

		if (argc == 1) {
			show_help(argv[0]);
			return 0;
		}

		for (int i = 1; i < argc; ++i) {
			std::string argvi = argv[i];
			if (argvi == "-h") {
				show_help(argv[0]);
				return 0;
			} else if (argvi == "-s") {
				list_mutable_state = true;
			} else if (argvi == "-t") {
				save_signal_time_stats = true;
			} else if (argvi == "-e") {
				enable_signal_stats = true;
			} else if (argvi == "-d") {
				disable_signal_stats = true;
			} else {
				std::cerr << "unknown argument: " << argvi << std::endl;
				return 1;
			}
		}
		// connect to saft-daemon
		auto connection = std::shared_ptr<saftbus::ProxyConnection>(new saftbus::ProxyConnection);

		if (list_mutable_state) {
			print_mutable_state(connection);
		}

		if (enable_signal_stats && disable_signal_stats) {
			std::cerr << "you can either disable or enable signal stats," << std::endl;
			std::cerr << " not both at the same time" << std::endl;
			return 1;
		}

		if (enable_signal_stats) {
			std::cout << "enabling signal flight time statistics in saftd" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_ENABLE_STATS);
		}
		if (disable_signal_stats) {
			std::cout << "disabling signal flight time statistics in saftd" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_DISABLE_STATS);
		}

		if (save_signal_time_stats) {
			std::cout << "downloading signal timing statistics from saftd" << std::endl;
			std::map<int, int> signal_flight_times;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_GET_STATS);
			saftbus::read(connection->get_fd(), signal_flight_times);
			std::ofstream statfile("signal_flight_times.dat");
			for (auto entry: signal_flight_times) {
				statfile << entry.first << " " << entry.second << std::endl;
			}
			statfile.close();
		}

		connection.reset();
		std::cerr << "saftbus-ctl done" << std::endl;
	} catch (...) {
		std::cout << "Error:" << std::endl;
		std::cout << "   Failed to connect to saftd. " << std::endl;
		std::cout << "   Either all sockets are busy, or saftd isn't running at all." << std::endl;
	}
	return 0;
}