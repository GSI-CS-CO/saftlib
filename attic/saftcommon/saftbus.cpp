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
#include "saftbus.h"
#include "core.h"
#include <iostream>

namespace saftbus
{

	const std::string socket_base_name = "/var/run/saftbus/saftbus";
	const char* saftbus_socket_environment_variable_name = "SAFTBUS_SOCKET_PATH";
	unsigned long device_msi_max_size = 0;
	unsigned long fg_fifo_max_size = 0;

	SlotBusAcquired   bus_acquired;
	SlotNameAcquired  name_acquired;
	SlotNameAppeared  name_appeared;
	SlotNameLost      name_lost;
	SlotNameVanished  name_vanished;
	std::shared_ptr<Connection> connection;


	unsigned own_name(BusType bus_type, 
	                  const std::string& name, 
	                  const SlotBusAcquired& bus_acquired_slot, 
	                  const SlotNameAcquired& name_acquired_slot, 
	                  const SlotNameLost& name_lost_slot)
	{
		init();
		if (_debug_level > 5) std::cerr << "own_name("<< bus_type<< "," << name <<") called" << std::endl;
		bus_acquired  = bus_acquired_slot;
		name_acquired = name_acquired_slot;
		name_lost     = name_lost_slot;

		// The Connection object will be created here.
		// It does what the DBus-daemon did in DBus based saftlib
		connection = std::shared_ptr<Connection>(new Connection);

		bus_acquired(connection, "bus acquired");
		name_acquired(connection, "name acquired");
		return 0;
	}

	void unown_name(unsigned id)
	{
		if (_debug_level > 5) std::cerr << "unown_name() called" << std::endl;
	}

}
