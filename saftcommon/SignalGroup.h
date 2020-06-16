/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef SIGNAL_GROUP_H
#define SIGNAL_GROUP_H

#include <poll.h>

#include <saftbus.h>

namespace saftlib
{


	class SignalGroup
	{
	public:
		void add(saftbus::Proxy *proxy, bool automatic_dispatch = true);
		void remove(saftbus::Proxy *proxy);
		int wait_for_signal(int timeout_ms = -1);
		bool sender_alive();

	private:
		std::vector<saftbus::Proxy* > _signal_group;
		std::vector<struct pollfd> _fds;

		bool _sender_alive;
	};

	// Block until a signal of a connected saftbus::Proxy arrives.
	// Connections between SignalGroup objects and proxies are established by
	// passing the SignalGroup object to the Proxy_create() function.
	// If no SignalGroup is connected explicitly, the globalSignalGroup is used.
	//
	// returns 0 if the timeout was hit
	//         nonzero otherwise
	// timeout is given in units of milliseconds
	// timeout of -1 means: no timeout
	int wait_for_signal(int timeout_ms = -1);

	extern SignalGroup globalSignalGroup;
	extern SignalGroup noSignals; // pass this to the Proxy if you don't want any signals

} // namespace saftlib

#endif

