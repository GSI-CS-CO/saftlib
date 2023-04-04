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
#include "service.hpp"

#include <cerrno>
#include <cstring>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

std::string print_fillstate();

void usage(char *argv0) {
		std::cout << "saftbusd version " << VERSION << std::endl;
		std::cout << std::endl;
		std::cout << "usage: " << argv0 << " [OPTIONS] { <plugin.so> { <plugin-arg> } }" << std::endl;
		std::cout << std::endl;
		std::cout << "  <plugin.so>     is the name of a shared object files, it must have" << std::endl; 
		std::cout << "                  contain a function with name \"create_services\"." << std::endl;
		std::cout << std::endl;
		std::cout << "  <plugin-arg>    one or more strings can be passed as arguments" << std::endl;
		std::cout << "                  to each plugin. They are arguments of the " << std::endl;
		std::cout << "                  \"create_services\" function in the shared library." << std::endl;
		std::cout << std::endl;
		std::cout << "options: " << std::endl;
		std::cout << std::endl;
		std::cout << " -h | --help      print this help and exit." << std::endl;
		std::cout << std::endl;
		std::cout << " -r <priority>    set scheduling policy to round robin with given priority." << std::endl;
		std::cout << "                  priority must be in the range [" << sched_get_priority_min(SCHED_RR) << " .. " << sched_get_priority_max(SCHED_RR) << "]" << std::endl;
		std::cout << std::endl;
		std::cout << " -f <priority>    set scheduling policy to fifo with given priority." << std::endl;
		std::cout << "                  priority must be in the range [" << sched_get_priority_min(SCHED_FIFO) << " .. " << sched_get_priority_max(SCHED_FIFO) << "]" << std::endl;
		std::cout << std::endl;
		std::cout << " -a <cpu>{,<cpu>} set affinity of this process to given cpus." << std::endl;
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

static bool set_realtime_scheduling(std::string argvi, char *prio) {
	std::istringstream in(prio);
	sched_param sp;
	in >> sp.sched_priority;
	if (!in) {
		std::cerr << "Error: cannot read priority from argument " << prio << std::endl;
		return false;
	}
	int policy = SCHED_RR;
	if (argvi == "-f") policy = SCHED_FIFO;
	if (sp.sched_priority < sched_get_priority_min(policy) && sp.sched_priority > sched_get_priority_max(policy)) {
		std::cerr << "Error: priority " << sp.sched_priority << " not supported " << std::endl;
		return false;
	} 
	if (sched_setscheduler(0, policy, &sp) < 0) {
		std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

static bool set_cpu_affinity(std::string argvi, char *affinity) {
	// affinity should be a comma-separated list such as "1,4,5,9"
	cpu_set_t set;
	CPU_ZERO(&set);
	std::string affinity_list = affinity;
	for(auto &ch: affinity_list) if (ch==',') ch=' ';
	std::istringstream in(affinity_list);
	int count = 0;
	for (;;) {
		int CPU;
		in >> CPU;
		if (!in) break;
		CPU_SET(CPU, &set);
		++count;
	}
	if (!count) {
		std::cerr << "Error: cannot read cpus from argument " << affinity << std::endl;
		return false;
	}
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
		std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}


int main(int argc, char *argv[]) {
	try {

		std::vector<std::pair<std::string, std::vector<std::string> > > plugins_and_args;
		for (int i = 1; i < argc; ++i) {
			std::string argvi(argv[i]);
			if (argvi == "-h" || argvi == "--help") {
				usage(argv[0]);
				return 0;
			} else if (argvi == "-r" || argvi == "-f" || argvi == "-a") {
				if (++i < argc) {
					if (argvi == "-a") {
						if (!set_cpu_affinity(argvi, argv[i])) return 1;
					} else {
						if (!set_realtime_scheduling(argvi, argv[i])) return 1;
					}
				} else {
					std::cerr << "Error: expect priority after " << argvi << std::endl;
					return 1;
				}
				continue;
			}
			bool argvi_is_plugin = (argvi.size()>3 && argvi.find(".so") == (argvi.size()-3));
			if (argvi_is_plugin) {
				std::cerr << argvi << " is plugin" << std::endl;
				plugins_and_args.push_back(std::make_pair(argvi, std::vector<std::string>()));
			} else {
				std::cerr << argvi << " is argument" << std::endl;
				if (plugins_and_args.empty()) {
					std::cerr << "Error: no plugin specified (these are files ending with .so) before argument " << argvi << std::endl;
					return 1;
				} else {
					plugins_and_args.back().second.push_back(argvi);
				}
			}
		}

		if (saftd_already_running()) {
			std::cerr << "Cannot start: saftbusd already running" << std::endl;
			return 1;
		}

		saftbus::ServerConnection server_connection(plugins_and_args);

		// add allocator fillstate as additional info to be reported by Container::get_status()
		if (print_fillstate().size()) server_connection.get_container()->add_additional_info_callback("allocator", &print_fillstate);

		saftbus::Loop::get_default().run();

		// delete all remaining source from Loop before the plugins are unloaded 
		saftbus::Loop::get_default().clear();

	} catch (std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		saftbus::Loop::get_default().clear();
		return 1;
	}

	return 0;
}