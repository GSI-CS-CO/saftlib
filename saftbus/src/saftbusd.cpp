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

#include "loop.hpp"
#include "server.hpp"
#include "client.hpp"
#include "global_allocator.hpp"

void usage(char *argv0) {
		std::cout << "saftbusd version " << VERSION << std::endl;
		std::cout << std::endl;
		std::cout << "usage: " << argv0 << " [-h] { <plugin.so> { <plugin-arg> } }" << std::endl;
		std::cout << std::endl;
		std::cout << "  <plugin.so>     is the name of a shared object files, it must have" << std::endl; 
		std::cout << "                  contain a function with name \"create_services\"." << std::endl;
		std::cout << std::endl;
		std::cout << "  <plugin-arg>    one or more strings can be passed as arguments" << std::endl;
		std::cout << "                  to each plugin. They are arguments of the " << std::endl;
		std::cout << "                  \"create_services\" function in the shared library." << std::endl;
		std::cout << std::endl;
		std::cout << " -h | --help      print this help and exit." << std::endl;
		std::cout << std::endl;
		
}


static bool saftd_already_running() 
{
  // if ClientConnection can be established, saftbus is already running
  try {
  	saftbus::ClientConnection test_connection;
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

int main(int argc, char *argv[]) {

	std::vector<std::pair<std::string, std::vector<std::string> > > plugins_and_args;
	for (int i = 1; i < argc; ++i) {
		std::string argvi(argv[i]);
		if (argvi == "-h" || argvi == "--help") {
			usage(argv[0]);
			return 0;
		}
		bool argvi_is_plugin = (argvi.find(".so") == argvi.size()-3);
		if (argvi_is_plugin) {
			plugins_and_args.push_back(std::make_pair(argvi, std::vector<std::string>()));
		} else {
			// std::cerr << argvi << "is argument" << std::endl;
			if (plugins_and_args.empty()) {
				std::cerr << "Error: no plugin specified (these are files ending with .so) before argument " << argvi << std::endl;
				return 1;
			} else {
				plugins_and_args.back().second.push_back(argvi);
			}
		}
	}

	if (saftd_already_running()) {
		std::cerr << "saftbusd already running" << std::endl;
		return 1;
	}

	saftbus::ServerConnection server_connection(plugins_and_args);
	saftbus::Loop::get_default().run();

	return 0;
}