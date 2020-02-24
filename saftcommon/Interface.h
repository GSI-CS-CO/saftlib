#ifndef SAFTBUS_INTERFACE_H_
#define SAFTBUS_INTERFACE_H_

#include <memory>
#include <sigc++/sigc++.h>
#include "Error.h"
#include "core.h"



namespace saftbus
{

	class Connection;

	class Message /*: public Glib::Object*/ {
	public:
		static std::shared_ptr<Message> create(const std::vector<int> &fds);
		//GUnixFDList* gobj(); // this is a bit of a hack, but simplifies things by a lot
		std::vector<int> &get_fd_list();
	private:
		Message(const std::vector<int> &fds);
		//std::shared_ptr<Gio::UnixFDList> _fd_list;
		std::vector<int> _fd_list;
	};

	class MethodInvocation /*: public Glib::Object*/
	{
	public:
		MethodInvocation();
		MethodInvocation(const std::vector<int> &fds);
		~MethodInvocation();

		void return_value(const Serial& parameters);
		void return_error(const saftbus::Error& error);

		std::shared_ptr<saftbus::Message>	get_message();

		Serial& get_return_value();
		saftbus::Error& get_return_error();
		bool has_error();
	private:
		Serial _parameters;
		saftbus::Error _error;
		bool _has_error = false;
		std::vector<int> _fds;
	};

	class InterfaceInfo /*: public Glib::Object*/ //Base
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
		using SlotInterfaceGetProperty = sigc::slot< void, Serial&, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string& >;
		using SlotInterfaceMethodCall = sigc::slot< void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Serial&, const std::shared_ptr<MethodInvocation>& >;
		using SlotInterfaceSetProperty = sigc::slot< bool, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Serial& >;
		InterfaceVTable 	( 	const SlotInterfaceMethodCall&  	slot_method_call,
								const SlotInterfaceGetProperty&  	slot_get_property = SlotInterfaceGetProperty(),
								const SlotInterfaceSetProperty&  	slot_set_property = SlotInterfaceSetProperty() 
			); 	
		SlotInterfaceGetProperty get_property;
		SlotInterfaceSetProperty set_property;
		SlotInterfaceMethodCall  method_call;
	};

	class NodeInfo /*: public Glib::Object*/ //Base
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
