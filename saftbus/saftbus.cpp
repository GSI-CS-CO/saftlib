#include "saftbus.h"

#include <iostream>

namespace saftbus
{

	SlotBusAcquired   bus_acquired;
	SlotNameAcquired  name_acquired;
	SlotNameAppeared  name_appeared;
	SlotNameLost      name_lost;
	SlotNameVanished  name_vanished;
	Glib::RefPtr<Connection> connection;


	guint own_name (BusType bus_type, const Glib::ustring& name, const SlotBusAcquired& bus_acquired_slot, const SlotNameAcquired& name_acquired_slot, const SlotNameLost& name_lost_slot)//, BusNameOwnerFlags flags=Gio::DBus::BUS_NAME_OWNER_FLAGS_NONE)
	{
		std::cerr << "own_name("<< bus_type<< "," << name <<") called" << std::endl;
		bus_acquired  = bus_acquired_slot;
		name_acquired = name_acquired_slot;
		name_lost     = name_lost_slot;

		connection = Glib::RefPtr<Connection>(new Connection);

		bus_acquired(connection, "blub bus bus_acquired");
		name_acquired(connection, "blub name acquired");
		//name_lost(connection, "blub name lost");
		return 0;
	}

	void unown_name(guint id)
	{
		std::cerr << "unown_name() called" << std::endl;

	}



}
