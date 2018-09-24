#ifndef SAFTBUS_H_
#define SAFTBUS_H_


#include <giomm.h>
#include <ctime>
#include <map>

#include "Logger.h"

namespace saftbus
{

	extern const int N_CONNECTIONS;

	enum BusType
	{
		BUS_TYPE_SESSION,
		BUS_TYPE_SYSTEM,
	};

	class Connection;

	using SlotBusAcquired = sigc::slot<void, const Glib::RefPtr<Connection>&, Glib::ustring>;
	using SlotNameAcquired = sigc::slot<void, const Glib::RefPtr<Connection>&, Glib::ustring>;
	using SlotNameAppeared = sigc::slot<void, const Glib::RefPtr<Connection>&, Glib::ustring, const Glib::ustring&>;
	using SlotNameLost = sigc::slot<void, const Glib::RefPtr<Connection>&, Glib::ustring>;
	using SlotNameVanished = sigc::slot<void, const Glib::RefPtr<Connection>&, Glib::ustring>;

	guint own_name (BusType bus_type, const Glib::ustring& name, const SlotBusAcquired& bus_acquired_slot=SlotBusAcquired(), const SlotNameAcquired& name_acquired_slot=SlotNameAcquired(), const SlotNameLost& name_lost_slot=SlotNameLost());//, BusNameOwnerFlags flags=Gio::DBus::BUS_NAME_OWNER_FLAGS_NONE);
	void unown_name(guint id);

	extern SlotBusAcquired   bus_acquired;
	extern SlotNameAcquired  name_acquired;
	extern SlotNameAppeared  name_appeared;
	extern SlotNameLost      name_lost;
	extern SlotNameVanished  name_vanished;
	extern Glib::RefPtr<Connection> connection;

	enum MessageTypeS2C {
		CLIENT_REGISTERED,
		METHOD_REPLY,
		METHOD_ERROR,
		SIGNAL,
		PORPERTY_CHANGED,
		PROPERTY_VALUE,
	};
	enum MessageTypeC2S {
		REGISTER_CLIENT,
		METHOD_CALL,
		PROPERTY_SET,
		PROPERTY_GET,
		SENDER_ID,
		SIGNAL_FD,
		SIGNAL_REMOVE_FD,
		SIGNAL_FLIGHT_TIME,

		SAFTBUS_CTL_HELLO,
		SAFTBUS_CTL_STATUS, // get saftbus status info
	};

	bool deserialize(Glib::Variant<std::vector<Glib::VariantBase> > &result, const char *data, gsize size);
	bool deserialize(Glib::Variant<Glib::VariantBase>               &result, const char *data, gsize size);


	struct Timer 
	{
		struct timespec start, stop;
		std::map<int, int> &hist;
		double delta_t() 
		{
		    clock_gettime( CLOCK_REALTIME, &stop);
			return (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
		         - (1.0e6*start.tv_sec   + 1.0e-3*start.tv_nsec);
		}	
		Timer(std::map<int, int> &h) : hist(h) {
			clock_gettime( CLOCK_REALTIME, &start);
		}
		~Timer() {
		    ++hist[delta_t()];
		}
	};

}


#include "Error.h"
#include "Interface.h"
#include "Connection.h"
#include "ProxyConnection.h"
#include "Proxy.h"



#endif
