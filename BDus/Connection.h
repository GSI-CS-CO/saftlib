#ifndef G10_BDUS_CONNECTION_H_
#define G10_BDUS_CONNECTION_H_

#include <giomm.h>

#include <map>

#include "Interface.h"

namespace G10
{

namespace BDus
{



	class Connection
	{

	public:

		Connection();

		void reference();
		void unreference();

		//guint 	register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info);
		guint 	register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable);
		bool 	unregister_object (guint registration_id);

		using SlotSignal = sigc::slot<void, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&>;

		guint signal_subscribe 	( 	const SlotSignal&  	slot,
									const Glib::ustring&  	sender = Glib::ustring(),
									const Glib::ustring&  	interface_name = Glib::ustring(),
									const Glib::ustring&  	member = Glib::ustring(),
									const Glib::ustring&  	object_path = Glib::ustring(),
									const Glib::ustring&  	arg0 = Glib::ustring()//,
									//SignalFlags  	flags = SIGNAL_FLAGS_NONE 
			);
		void signal_unsubscribe 	( 	guint  	subscription_id	) ;

		void 	emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name=Glib::ustring(), const Glib::VariantContainerBase& parameters=Glib::VariantContainerBase());


		Glib::VariantContainerBase call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name=Glib::ustring(), int timeout_msec=-1);


		std::vector<std::string> _bdus_objects;
		int counter = 0;

	};

}

}

#endif
