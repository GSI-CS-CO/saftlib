// @brief Retrieve the name of the etherbone path of a logical saftlib device (eg. tr0)
// @author Michael Reese  <m.reese@gsi.de>
//
// Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH
//
// Some etherbone devices support only a single connection. If saftlib
// connects to such a device, it will open a pseudo-terminal (e.g. /dev/pts/<number>).
// It will listen to data on the pseudo-terminal and make it behave like
// a real etherbone slave by redirecting the traffic to the connected hardware
// and deliver the response.
//
// Usage scenario: saftlib is connected to /dev/ttyUSB0 under logical name tr0.
//                 If "eb-ls dev/ttyUSB0" would be called now, both saftlib and
//                 the device will crash, because the serial driver doesn't 
//                 support multiple connections.
//                 If instead "eb-ls $(saft-eb-fwd tr0)" is called, eb-ls will
//                 work as expected.
//
//*****************************************************************************
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
//
#include "interfaces/SAFTd.h"
#include <iostream>

int main(int argc, char *argv[])
{
	try {
		if (argc != 2) {
			std::cerr << "usage: " << argv[0] << " saftlib-device" << std::endl;
			std::cerr << std::endl;
			std::cerr << "example " << argv[0] << " tr0" << std::endl;
			return 1;
		}
    	std::shared_ptr<saftlib::SAFTd_Proxy> saftd = saftlib::SAFTd_Proxy::create();
    	std::cout << saftd->EbForward(std::string(argv[1])) << std::endl;
	} catch (saftbus::Error &e) {
		std::cerr << e.what() << std::endl;
		return 2;
	}
	return 0;
}
