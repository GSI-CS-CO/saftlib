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

    /// @brief The class is constructed with the name of a shared library file (.so). It opens the file, 
    ///        gets the address of the function symbol "create_services". The public member function
    ///        create_services forwards all arguments to the loaded function from the shared library.
    /// 
    /// The create_services function in the shared library can use the container pointer to add
    /// Service objects into the container.
	class LibraryLoader {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		LibraryLoader(const std::string &so_filename);
		~LibraryLoader();
        /// @brief call the create_service function of the shared library associated with this object (the so_filename passed to the constructor).
        ///
        /// @param container normally, the create_service function in the shared library will use this pointer to add Service objects into the container.
        /// @param args a vector of string arguments. The meaning of the arguments depends on the shared library.
		/// the service objects are owned by the plugin, we only get a naked pointer to it
		/// the std::string is the object path under which the respective Service object should be mounted		
		void create_services(Container *container, const std::vector<std::string> &args = std::vector<std::string>()); 
	};

}



#endif
