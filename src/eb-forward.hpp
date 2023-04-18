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

#ifndef SAFTLIB_EB_FORWARD_H
#define SAFTLIB_EB_FORWARD_H

#include <string>
#include <vector>
#include <map>

#include <saftbus/loop.hpp>

#include "SdbDevice.hpp"

namespace saftlib {

    /// @brief Maintains a pseudo-terminal device that mimics a serial etherbone device.
    ///
    /// All data from the created pseudo terminal (e.g. /dev/pts/14) is read, split-up into complete 
    /// etherbone packages and redirected to the real hardware. When the real hardware responds to the
    /// etherbone request, the response is written back to the pseudo-terminal device.
    /// This effectively allows using eb-tools, such as eb-ls on serial devices, even when the device is
    /// occupied the TimingReceiver object.
	class EB_Forward : public SdbDevice {
	public:
		EB_Forward(const std::string& eb_name, etherbone::Device &device); 
		~EB_Forward();

		bool accept_connection(int condition);

        /// @brief return the name of the pseudo-terminal device
        /// @return the name of the created pseudo-terminal
		std::string eb_forward_path();

	private:
		void write_all(int fd, char *ptr, int size);
		void read_all(int fd, char *ptr, int size);
		void open_pts();
		int     _eb_device_fd, _pts_fd; 
        saftbus::SourceHandle io_source;

        std::vector<std::pair<std::string, std::string> > visu;

        std::vector<uint8_t> request;  // data from eb-tool
        std::vector<uint8_t> response; // data from device

	};


}

#endif
