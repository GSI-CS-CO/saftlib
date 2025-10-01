/*  Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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


#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"
#include "Time.hpp"

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include <saftbus/error.hpp>

#include <dirent.h>


std::unique_ptr<saftlib::SAFTd> saftd;
              
extern "C" 
void destroy_service() {
	saftd.reset(); // destructor + release memory
}

/// @brief if the name and etherbone_path have '*' as last charater, the etherbone_path is scanned for matching device files. All matching devices will be attached.
///
/// If no match is found, nothing is attached.
/// If no '*' is found, the device is attached directly.
/// @param saftd a pointer to a SAFTd
/// @param name logical saftlib name. For example tr0, tr1 or tr*
/// @param etherbone_path etherbone path. If name has a '*' as last character, etherbone_path needs '*' as last character, too.
/// @param poll_interval_ms this is directly passed to AttachDevice function
void handle_wildcards_and_attach_device(saftlib::SAFTd *saftd, const std::string name, const std::string etherbone_path, int poll_interval_ms) {
	if (name.size() && name.back() == '*') {
		if (etherbone_path.size() && etherbone_path.back() != '*') {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "if name has * wildcard as last char, etherbone_path also needs wildcard as last char");
		}
		std::string path("/");
		path.append(etherbone_path.substr(0,etherbone_path.find_last_of('/')));

		bool found_one = false;
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(path.c_str())) != nullptr) {
			try {
				while ((ent = readdir(dir)) != nullptr) {
					std::string entry(path);
					entry.append("/");
					entry.append(ent->d_name);
					if (entry.substr(1,etherbone_path.size()-1) == etherbone_path.substr(0,etherbone_path.size()-1)) {						
						std::string number = entry.substr(etherbone_path.size());
						std::string new_name = name.substr(0,name.size()-1);
						new_name.append(number);
						std::string new_path = etherbone_path.substr(0,etherbone_path.size()-1);
						new_path.append(number);

						std::cerr << "found name device pair "  << new_name << ":" << new_path << std::endl;
						found_one = true;
						saftd->AttachDevice(new_name, new_path, poll_interval_ms);
					}
				}
				if (!found_one) {
					std::cerr << "no device matches " << etherbone_path << std::endl;
				}
			} catch (...) {
				closedir(dir);
				throw;
			}
		} else {
			std::ostringstream msg;
			msg << "cannot open dirctory " << path;
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg.str());
		}
	} else {
		saftd->AttachDevice(name, etherbone_path, poll_interval_ms);
	}
}



extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	// There can only be one saftd.
	if (saftd) {
		throw std::runtime_error("service alreayd exists");
	}

	// Initialize leap second list eagerly and thread-safely before any timing services are created
	// This ensures consistent performance for time requests and catches configuration errors early
	saftlib::init();

	saftd = std::unique_ptr<saftlib::SAFTd>(new saftlib::SAFTd(container));
	// create a new Service and return it. Maintain a reference count
	container->create_object(saftd->getObjectPath(), std::move(std::unique_ptr<saftlib::SAFTd_Service>(new saftlib::SAFTd_Service(saftd.get(), std::bind(&destroy_service), false ))));

	// attach devices as specified in args
	// this must happen after the SAFTd_Service was moved to the saftbus::Container
	// saftbus::Container remembers the order of creation of all services and in case of shutdown, 
	// it destroyes them in reverse order. If SAFTd_Service is destroyed before the attached devices 
	// then destroying the attached devices will result in segmentation faults (because the destruction_callback
	// is part of SAFTd_Service which doesnt exist anymore)
	for (auto &arg: args) {
		size_t pos = arg.find(':'); // the position of the first colon ':'
		if (pos == arg.npos || pos+1 == arg.size()) {
			throw std::runtime_error("expect <name>:<eb-path>[:<poll-interval>] as argument");
		}
		std::string name = arg.substr(0, pos);
		std::string path = arg.substr(pos+1);
		int poll_interval_ms = 1;
		size_t pos2 = path.find(':'); // the position of the second colon ':'
		if (pos2 != path.npos) {
			if (pos2+1 == path.size()) { // 2nd colon is there, but poll inteval is missing
				throw std::runtime_error("expect <name>:<eb-path>[:<poll-interval>] as argument");
			} 
			std::istringstream poll_interval(path.substr(pos2+1));
			poll_interval >> poll_interval_ms;
			if (!poll_interval || poll_interval_ms <= 0) {
				std::ostringstream msg;
				msg << "cannot read poll interval from \'" << path.substr(pos2+1) << "\' after " << name << ":" << path << ":";
				throw std::runtime_error(msg.str());
			}
			path = path.substr(0,pos2);
		}
		handle_wildcards_and_attach_device(saftd.get(), name, path, poll_interval_ms);
	}
}


