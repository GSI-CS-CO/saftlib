#include "saftbus.h"
#include "core.h"
#include <iostream>

namespace saftbus
{

	const int N_CONNECTIONS = 32;

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
