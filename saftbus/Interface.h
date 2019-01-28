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
		InterfaceInfo(const std::string &interface_name);
		const std::string &get_interface_name();
	private:
		std::string _interface_name;
	};


	class InterfaceVTable
	{
	public:

		using SlotInterfaceGetProperty = sigc::slot< void, Glib::VariantBase&, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string& >;
		using SlotInterfaceMethodCall = sigc::slot< void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Glib::VariantContainerBase&, const std::shared_ptr<MethodInvocation>& >;
		using SlotInterfaceSetProperty = sigc::slot< bool, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Glib::VariantBase& >;
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
		NodeInfo(const std::string &interface_name);
		static std::shared_ptr<NodeInfo> create_for_xml(const std::string&  xml_data);
		std::shared_ptr<InterfaceInfo> lookup_interface();
	private:
		std::shared_ptr<InterfaceInfo> _interface_info;
	};


}


#endif
