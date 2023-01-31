/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#include "client.hpp"

#include <iostream>
#include <algorithm>
#include <thread>

void print_status(saftbus::SaftbusInfo &saftbus_info) {
	auto max_object_path_length = std::string().size();
	for (auto &object: saftbus_info.object_infos) {
		max_object_path_length = std::max(max_object_path_length, object.object_path.size());
	}

	for (auto &object: saftbus_info.object_infos) {
		std::cout << std::setw(max_object_path_length) << std::left 
		          << object.object_path 
		          << " " 
		          << object.object_id
		          << " [" << object.owner << "] ";
		for (auto &user: object.signal_fds_use_count) {
			std::cout << user.first << "/" << user.second << " ";
		}
		for (auto &interface: object.interface_names) {
			std::cout << interface << " ";
		}		          
        std::cout << std::endl;
	}

	std::cout << std::endl;
	for (auto &client: saftbus_info.client_infos) {
		std::cout << client.client_fd << " (pid=" << client.process_id << ")" << std::endl;
	}

}

int main(int argc, char **argv)
{
	try {

		auto container_proxy = saftbus::Container_Proxy::create();

		if (argc > 1) {
			for (int i = 1; i < argc; ++i) {
				std::string argvi(argv[i]);
				if (argvi == "-q") {
					//===std::cerr << "call proxy->quit()" << std::endl;
					container_proxy->quit();
					return 0;
					//===std::cerr << "quit done" << std::endl;
				} if (argvi == "-s") {
					saftbus::SaftbusInfo saftbus_info = container_proxy->get_status();
					print_status(saftbus_info);
					return 0;
				} else if (argvi == "-l") {
					if ((++i) < argc) {
						std::string so_filename = argv[i];
						std::vector<std::string> plugin_args;
						for (++i; i < argc; ++i) {
							std::cerr << "add arg : " << argv[i] << std::endl;
							plugin_args.push_back(argv[i]);
						}
						container_proxy->load_plugin(so_filename, plugin_args);
						return 1;
					} else {
						throw std::runtime_error("expect so-filename after -l");
					}
				} else if (argvi == "-u") {
					if ((++i) < argc) {
						std::string so_filename = argv[i];
						std::vector<std::string> plugin_args;
						for (++i; i < argc; ++i) {
							std::cerr << "add arg : " << argv[i] << std::endl;
							plugin_args.push_back(argv[i]);
						}
						container_proxy->unload_plugin(so_filename, plugin_args);
						return 1;
					} else {
						throw std::runtime_error("expect so-filename after -u");
					}
				} else if (argvi == "-r") {
					if ((i+=1) < argc) {
						container_proxy->remove_object(argv[i]);
					} else {
						throw std::runtime_error("expect object_path -r");
					}
				} else {
					std::string msg("unknown argument: ");
					msg.append(argvi);
					throw std::runtime_error(msg);
				}
			}
		} else {
			
			for (int i=0; i<10; ++i) {
				saftbus::SignalGroup::get_global().wait_for_signal();
			}
		}

	} catch (std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}