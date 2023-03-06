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

#ifndef SAFTBUS_PLUGINS_HPP_
#define SAFTBUS_PLUGINS_HPP_

#include "service.hpp"
#include "loop.hpp"

#include <string>
#include <vector>
#include <map>

namespace saftbus {

	class LibraryLoader {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		LibraryLoader(const std::string &so_filename);
		~LibraryLoader();
		// the service objects are owned by the plugin, we only get a naked pointer to it
		// the std::string is the object path under which the respective Servie object should be mounted		
		void create_services(Container *container, const std::vector<std::string> &args = std::vector<std::string>()); 
	};

}



#endif
