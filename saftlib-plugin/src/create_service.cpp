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


#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <sstream>

std::unique_ptr<saftlib::SAFTd> saftd;
int ref_count = 0;
                                                                                    
extern "C" 
void destroy_service() {
	--ref_count;
	std::cerr << "destroy_service was called " << ref_count << std::endl;
	// this is a hint, that the Service object will no longer be needed an we can (if we want to) savely destroy the SAFTd object
	if (ref_count == 0) {
		std::cerr << " destroying service" << std::endl;
		saftd.reset(); // destructor + release memory
	}
}

extern "C" 
void create_services(saftbus::Container *container, const std::vector<std::string> &args) {
	if (!saftd) {
		// There can only be one saftd.
		// Always return the same instance of saftd.
		saftd         = std::move(std::unique_ptr<saftlib::SAFTd>(new saftlib::SAFTd(container)));
	}

	// create a new Service and return it. Maintain a reference count
	++ref_count;
	container->create_object(saftd->getObjectPath(), std::move(std::unique_ptr<saftlib::SAFTd_Service>(new saftlib::SAFTd_Service(saftd.get(), std::bind(&destroy_service)))));

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
		int poll_interval_ms;
		size_t pos2 = path.find(':'); // the position of the second colon ':'
		if (pos2 != path.npos) {
			if (pos2+1 == path.size()) { // 2nd colon is there, but poll inteval is missing
				throw std::runtime_error("expect <name>:<eb-path>[:<poll-interval>] as argument");
			} 
			std::istringstream poll_interval(path.substr(pos2+1));
			poll_interval >> poll_interval_ms;
			if (!poll_interval || poll_interval_ms <= 0) {
				throw std::runtime_error("cannot read poll interval. must be a positive integer");
			}
			path = path.substr(0,pos2);
		}
		saftd->AttachDevice(name, path, poll_interval_ms);
	}
}


