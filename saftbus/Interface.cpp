#include "Interface.h"
#include "Error.h"

#include <iostream>
#include <algorithm>
#include "core.h"

namespace saftbus
{

Glib::RefPtr<Message> Message::create(const std::vector<int> &fds)
{
	return Glib::RefPtr<Message>(new Message(fds));;
}


Message::Message(const std::vector<int> &fds)
{
	_fd_list = Gio::UnixFDList::create();
	for(auto fd: fds) {
		_fd_list->append(fd);
	}
}

GUnixFDList* Message::gobj()
{
	return _fd_list->gobj();
}

InterfaceVTable::InterfaceVTable ( 	const SlotInterfaceMethodCall&  slot_method_call,
									const SlotInterfaceGetProperty&  slot_get_property,
									const SlotInterfaceSetProperty&  slot_set_property 
									)
	: get_property(slot_get_property)
	, set_property(slot_set_property)
	, method_call(slot_method_call)
{
	if (_debug_level > 5) std::cerr << "InterfaceVTable::InterfaceVTable() called" << std::endl;
}

InterfaceInfo::InterfaceInfo(const std::string &interface_name)
	: _interface_name(interface_name)
{}
const std::string &InterfaceInfo::get_interface_name()
{
	return _interface_name;
}

MethodInvocation::MethodInvocation()
{}

MethodInvocation::MethodInvocation(const std::vector<int> &fds)
	: _fds(fds)
{}

MethodInvocation::~MethodInvocation()
{
	for (auto fd: _fds) {
		close(fd);
	}
}


void MethodInvocation::return_value	(const Glib::VariantContainerBase& parameters)
{
	if (_debug_level > 5) std::cerr << "MethodInvocation::return_value()" << std::endl;
	if (_debug_level > 5) std::cerr << "   parameters  = " << parameters.print() << std::endl;
	_parameters = parameters;
	if (_debug_level > 5) std::cerr << "   _parameters = " << _parameters.print() << std::endl;
}
void MethodInvocation::return_error	(const saftbus::Error& error)
{
	_has_error = true;
	_error = error;
}

Glib::RefPtr<saftbus::Message>	MethodInvocation::get_message()
{
	Glib::RefPtr<Message> msg = Message::create(_fds);
	return msg;
}


Glib::VariantContainerBase& MethodInvocation::get_return_value()
{
	if (_debug_level > 5) std::cerr << "MethodInvocation::get_return_value()" << std::endl;
	if (_debug_level > 5) std::cerr << "   _parameters = " << _parameters.print() << std::endl;
	return _parameters;
}
bool MethodInvocation::has_error() 
{
	return _has_error;
}
saftbus::Error& MethodInvocation::get_return_error()
{
	return _error;
}




NodeInfo::NodeInfo(const std::string &interface_name)
	: _interface_info(new InterfaceInfo(interface_name))
{}

std::shared_ptr<NodeInfo> NodeInfo::create_for_xml (const std::string&  xml_data)
{
	// This function only extracts the interface name from the xml interface description
	// The rest is not needed by saftbus.
	if (_debug_level > 5) std::cerr << "NodeInfo::create_for_xml() called" << std::endl;
	std::string::size_type pos_begin = xml_data.find("interface name=\'");
	if (_debug_level > 5) std::cerr << " pos_begin = " << pos_begin << std::endl;
	if (pos_begin != xml_data.npos) {
		std::string::size_type pos_end = xml_data.find("\'",pos_begin+16u);
		if (_debug_level > 5) std::cerr << " pos_end = " << pos_end << std::endl;
		std::string interface_name = xml_data.substr(pos_begin+16u,pos_end-pos_begin-16u);
		if (_debug_level > 5) std::cerr << "found interface name " << interface_name << std::endl;
		return std::shared_ptr<NodeInfo>(new NodeInfo(interface_name));
	}
	return std::shared_ptr<NodeInfo>();
}
std::shared_ptr<InterfaceInfo> NodeInfo::lookup_interface ()
{
	if (_debug_level > 5) std::cerr << "NodeInfo::lookup_interface () called" << std::endl;
	return _interface_info;
}

}
