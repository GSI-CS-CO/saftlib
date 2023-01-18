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
#include "chunck_allocator_rt.hpp"

int main(int argc, char *argv[]) {

	std::vector<std::pair<std::string, std::vector<std::string> > > plugins_and_args;
	for (int i = 1; i < argc; ++i) {
		// std::cerr << argv[i] << std::endl;
		std::string argvi(argv[i]);
		bool argvi_is_plugin = //(argvi.find(".la") == argvi.size()-3) || 
		                       (argvi.find(".so") == argvi.size()-3);
		if (argvi_is_plugin) {
			// std::cerr << argvi << "is plugin name" << std::endl;
			plugins_and_args.push_back(std::make_pair(argvi, std::vector<std::string>()));
		} else {
			// std::cerr << argvi << "is argument" << std::endl;
			if (plugins_and_args.empty()) {
				// std::cerr << "no plugin specified (these are files ending with .la or .so)" << std::endl;
				return 1;
			} else {
				plugins_and_args.back().second.push_back(argvi);
			}
		}
	}

	saftbus::ServerConnection server_connection(plugins_and_args);
	saftbus::Loop::get_default().run();

	//===std::cerr << "===================== saftbusd quit ============================" << std::endl;
	return 0;
}