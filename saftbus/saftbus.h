#ifndef SAFTBUS_H_
#define SAFTBUS_H_


#include <sigc++/sigc++.h>
#include <memory>
#include <cstdint>
#include <ctime>
#include <map>

#include "Logger.h"

namespace saftbus
{

	extern const int N_CONNECTIONS;

	static const std::string socket_base_dir = "/var/run/saftbus";
	static const std::string socket_base_name = socket_base_dir+"/saftbus_";

	enum BusType
	{
		BUS_TYPE_SESSION,
		BUS_TYPE_SYSTEM,
	};

	class Connection;

	using SlotBusAcquired = sigc::slot<void, const std::shared_ptr<Connection>&, std::string>;
	using SlotNameAcquired = sigc::slot<void, const std::shared_ptr<Connection>&, std::string>;
	using SlotNameAppeared = sigc::slot<void, const std::shared_ptr<Connection>&, std::string, const std::string&>;
	using SlotNameLost = sigc::slot<void, const std::shared_ptr<Connection>&, std::string>;
	using SlotNameVanished = sigc::slot<void, const std::shared_ptr<Connection>&, std::string>;

	unsigned own_name (BusType bus_type, const std::string& name, const SlotBusAcquired& bus_acquired_slot=SlotBusAcquired(), const SlotNameAcquired& name_acquired_slot=SlotNameAcquired(), const SlotNameLost& name_lost_slot=SlotNameLost());//, BusNameOwnerFlags flags=Gio::DBus::BUS_NAME_OWNER_FLAGS_NONE);
	void unown_name(unsigned id);

	extern SlotBusAcquired   bus_acquired;
	extern SlotNameAcquired  name_acquired;
	extern SlotNameAppeared  name_appeared;
	extern SlotNameLost      name_lost;
	extern SlotNameVanished  name_vanished;
	extern std::shared_ptr<Connection> connection;

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
		// METHOD_CALL_ASYNC,
		PROPERTY_SET,
		PROPERTY_GET,
		SENDER_ID,
		SIGNAL_FD,
		SIGNAL_REMOVE_FD,
		SIGNAL_FLIGHT_TIME,

		SAFTBUS_CTL_ENABLE_STATS,
		SAFTBUS_CTL_DISABLE_STATS,
		SAFTBUS_CTL_GET_STATS,
		SAFTBUS_CTL_HELLO,
		SAFTBUS_CTL_STATUS, // get saftbus status info
		SAFTBUS_CTL_GET_STATE, 
		SAFTBUS_CTL_ENABLE_LOGGING,
		SAFTBUS_CTL_DISABLE_LOGGING,

		GET_SAFTBUS_INDEX,
	};

	// bool deserialize(Glib::Variant<std::vector<Glib::VariantBase> > &result, const char *data, gsize size);
	// bool deserialize(Glib::Variant<Glib::VariantBase>               &result, const char *data, gsize size);


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
#include "SignalGroup.h"



#endif
