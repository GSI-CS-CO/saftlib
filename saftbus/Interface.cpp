#include "Interface.h"

#include <iostream>
#include <algorithm>

namespace saftbus
{


void MethodInvocation::return_value (const Glib::VariantContainerBase& 	parameters)
{

}
void MethodInvocation::return_error (const saftbus::Error& error) 
{

}



InterfaceVTable::InterfaceVTable ( 	const SlotInterfaceMethodCall&  slot_method_call,
									const SlotInterfaceGetProperty&  slot_get_property,
									const SlotInterfaceSetProperty&  slot_set_property 
									)
	: get_property(slot_get_property)
	, set_property(slot_set_property)
	, method_call(slot_method_call)
{
	std::cerr << "InterfaceVTable::InterfaceVTable() called" << std::endl;
}

InterfaceInfo::InterfaceInfo(const Glib::ustring &interface_name)
	: _interface_name(interface_name)
{}
const Glib::ustring &InterfaceInfo::get_interface_name()
{
	return _interface_name;
}


NodeInfo::NodeInfo(const Glib::ustring &interface_name)
	: _interface_info(new InterfaceInfo(interface_name))
{}

Glib::RefPtr<NodeInfo> NodeInfo::create_for_xml (const Glib::ustring&  xml_data)
{
	std::cerr << "NodeInfo::create_for_xml() called" << std::endl;
	//std::find(xml_data.begin(), xml_data.end(), "interface name=\'")
	Glib::ustring::size_type pos_begin = xml_data.find("interface name=\'");
	std::cerr << " pos_begin = " << pos_begin << std::endl;
	if (pos_begin != xml_data.npos) {
		Glib::ustring::size_type pos_end = xml_data.find("\'",pos_begin+16u);
		std::cerr << " pos_end = " << pos_end << std::endl;
		Glib::ustring interface_name = xml_data.substr(pos_begin+16u,pos_end-pos_begin-16u);
		std::cerr << "found interface name " << interface_name << std::endl;
		return Glib::RefPtr<NodeInfo>(new NodeInfo(interface_name));
	}
	return Glib::RefPtr<NodeInfo>();
}
Glib::RefPtr<InterfaceInfo> NodeInfo::lookup_interface ()
{
	std::cerr << "NodeInfo::lookup_interface () called" << std::endl;
	return _interface_info;
}

}
