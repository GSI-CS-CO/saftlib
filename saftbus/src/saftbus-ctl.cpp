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


void usage(char *argv0) {
		std::cout << "saftbus-ctl version " << VERSION << std::endl;
		std::cout << std::endl;
		std::cout << "usage: " << argv0 << " [-s] [-r <object-path>] [-l <plugin.so> {plugin-args}] [-u <plugin.so>] [-h|--help]" << std::endl;
		std::cout << std::endl;
		std::cout << "  -s           print saftbus status, i.e. all available services," << std::endl; 
		std::cout << "               loaded plugins and connected clients." << std::endl;
		std::cout << std::endl;
		std::cout << "  -r           remove the service with <object-path> from saftbus" << std::endl; 
		std::cout << std::endl;
		std::cout << "  -l           load a share object file as plugin and execute the create_services" << std::endl;
		std::cout << "               function of the plugin. If the plugin was loaded before, the" << std::endl;
		std::cout << "               create_services function is called again. plugin-args are " << std::endl;
		std::cout << "               passed as arguments to the create_services function." << std::endl;
		std::cout << "               The meaning of plugin-args depends on the plugin (see plugin documentation)." << std::endl;
		std::cout << std::endl;
		std::cout << "  -u           unload plugin. This should only be done when no services" << std::endl;
		std::cout << "               that were created from code within that plugin are active." << std::endl;
		std::cout << std::endl;
		std::cout << "  -q           cause saftbusd to quit" << std::endl;
		std::cout << std::endl;
		std::cout << "  -h | --help  print help and exit" << std::endl;
		std::cout << std::endl;
}


void print_status(saftbus::SaftbusInfo &saftbus_info) {
	auto max_object_path_length = std::string().size();
	for (auto &object: saftbus_info.object_infos) {
		max_object_path_length = std::max(max_object_path_length, object.object_path.size());
	}
	std::cout << "services:" << std::endl;
	std::cout << "  " << std::setw(max_object_path_length) << std::left << "object-path" 
	          << " ID [owner] sig-fd/use-count interface-names" << std::endl;
	std::vector<std::string> services;	          
	for (auto &object: saftbus_info.object_infos) {
		std::ostringstream line;
		line << "  " 
		     << std::setw(max_object_path_length) << std::left 
		     << object.object_path 
		     << " " 
		     << object.object_id
		     << " [" << object.owner << "]";
		     if (object.has_destruction_callback &&  object.destroy_if_owner_quits) line << "D ";
		else if (object.has_destruction_callback && !object.destroy_if_owner_quits) line << "d ";
		else line << "  ";

		for (auto &user: object.signal_fds_use_count) {
			line << user.first << "/" << user.second << " ";
		}
		for (auto &interface: object.interface_names) {
			line << interface << " ";
		}		          
		services.push_back(line.str());
	}

	std::sort(services.begin(), services.end());
	for (auto &service: services) {
		std::cout << service << std::endl;
	}

	std::cout << std::endl;
	std::cout << "active plugins: " << std::endl;
	for (auto &plugin: saftbus_info.active_plugins) {
		std::cout << "  " << plugin << std::endl;
	}


	std::cout << std::endl;
	std::cout << "connected client processes:" << std::endl;
	for (auto &client: saftbus_info.client_infos) {
		std::cout << "  " << client.client_fd << " (pid=" << client.process_id << ")" << std::endl;
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
					std::string answer;
					std::cout << std::endl;
					std::cout << "are you sure you want to quit saftbusd [YES/no]? ";
					std::cout << std::endl;
					std::cin >> answer;
					if (answer == "YES") {
						container_proxy->quit();
						std::cout << "shutdown saftbusd " << std::endl;
					} else {
						std::cout << "did not shoutdown saftbusd" << std::endl;
					}
					return 0;
				} 
				if (argvi == "-h" || argvi == "--help") {
					usage(argv[0]);
					return 0;
				} 
				if (argvi == "-s") {
					saftbus::SaftbusInfo saftbus_info = container_proxy->get_status();
					print_status(saftbus_info);
					return 0;
				}
				if (argvi == "-l") {
					if ((++i) < argc) {
						std::string so_filename = argv[i];
						std::vector<std::string> plugin_args;
						for (++i; i < argc; ++i) {
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
			usage(argv[0]);
			return 0;
		}

	} catch (std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}