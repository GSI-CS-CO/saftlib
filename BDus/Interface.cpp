#include "Interface.h"

#include <iostream>


namespace G10 
{

namespace BDus
{


void MethodInvocation::return_value (const Glib::VariantContainerBase& 	parameters)
{

}
void MethodInvocation::return_error (const G10::BDus::Error& error) 
{

}


void InterfaceInfo::reference()
{
}
void InterfaceInfo::unreference()
{
}


InterfaceVTable::InterfaceVTable ( 	const SlotInterfaceMethodCall&  slot_method_call,
									const SlotInterfaceGetProperty&  slot_get_property,
									const SlotInterfaceSetProperty&  slot_set_property 
									)
{
	std::cerr << "InterfaceVTable::InterfaceVTable() called" << std::endl;
}

void NodeInfo::reference()
{

}
void NodeInfo::unreference()
{

}

Glib::RefPtr<NodeInfo> NodeInfo::create_for_xml (const Glib::ustring&  xml_data)
{
	std::cerr << "NodeInfo::create_for_xml() called" << std::endl;
}
Glib::RefPtr<InterfaceInfo> NodeInfo::lookup_interface ()
{
	std::cerr << "NodeInfo::lookup_interface () called" << std::endl;
}

}
}