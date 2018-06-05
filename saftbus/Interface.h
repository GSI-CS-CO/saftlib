#ifndef G10_BDUS_INTERFACE_H_
#define G10_BDUS_INTERFACE_H_


#include <giomm.h>

namespace saftbus
{

	class Connection;
	class Error;

	class MethodInvocation
	{
	public:
		void return_value 	( 	const Glib::VariantContainerBase&  	parameters	) ;
		void return_error 	( 	const saftbus::Error&  	error	) 	;
	};


	class InterfaceInfo
	{
	public:
		void reference();
		void unreference();

	};


	class InterfaceVTable
	{
	public:

		using SlotInterfaceGetProperty = sigc::slot< void, Glib::VariantBase&, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring& >;
		using SlotInterfaceMethodCall = sigc::slot< void, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&, const Glib::RefPtr<MethodInvocation>& >;
		using SlotInterfaceSetProperty = sigc::slot< bool, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantBase& >;
		InterfaceVTable 	( 	const SlotInterfaceMethodCall&  	slot_method_call,
								const SlotInterfaceGetProperty&  	slot_get_property = SlotInterfaceGetProperty(),
								const SlotInterfaceSetProperty&  	slot_set_property = SlotInterfaceSetProperty() 
			); 	
	};

	class NodeInfo
	{
	public:
		void reference();
		void unreference();

		static Glib::RefPtr<NodeInfo> create_for_xml (const Glib::ustring&  xml_data); 	
		//Glib::RefPtr<InterfaceInfo> lookup_interface (const Glib::ustring&  name);
		Glib::RefPtr<InterfaceInfo> lookup_interface ();
	};


}


#endif
