#ifndef SAFTBUS_INTERFACE_H_
#define SAFTBUS_INTERFACE_H_


#include <giomm.h>
#include "Error.h"




namespace saftbus
{

	class Connection;

	class Message : public Glib::Object {
	public:
		static Glib::RefPtr<Message> create(const std::vector<int> &fds);
		GUnixFDList* gobj(); // this is a bit of a hack, but simplifies things by a lot
	private:
		Message(const std::vector<int> &fds);
		Glib::RefPtr<Gio::UnixFDList> _fd_list;
	};

	class MethodInvocation : public Glib::Object
	{
	public:
		MethodInvocation();
		MethodInvocation(const std::vector<int> &fds);
		~MethodInvocation();

		void return_value(const Glib::VariantContainerBase& parameters);
		void return_error(const saftbus::Error& error);

		Glib::RefPtr<saftbus::Message>	get_message();

		Glib::VariantContainerBase& get_return_value();
		saftbus::Error& get_return_error();
		bool has_error();
	private:
		Glib::VariantContainerBase _parameters;
		saftbus::Error _error;
		bool _has_error = false;
		std::vector<int> _fds;
	};

	class InterfaceInfo : public Glib::Object//Base
	{
	public:
		InterfaceInfo(const Glib::ustring &interface_name);
		const Glib::ustring &get_interface_name();
	private:
		Glib::ustring _interface_name;
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
		SlotInterfaceGetProperty get_property;
		SlotInterfaceSetProperty set_property;
		SlotInterfaceMethodCall  method_call;
	};

	class NodeInfo : public Glib::Object//Base
	{
	public:
		NodeInfo(const Glib::ustring &interface_name);
		static Glib::RefPtr<NodeInfo> create_for_xml(const Glib::ustring&  xml_data);
		Glib::RefPtr<InterfaceInfo> lookup_interface();
	private:
		Glib::RefPtr<InterfaceInfo> _interface_info;
	};


}


#endif