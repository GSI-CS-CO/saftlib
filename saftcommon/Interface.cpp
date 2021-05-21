/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#include "Interface.h"
#include "Error.h"

#include <unistd.h>
#include <iostream>
#include <algorithm>
#include "core.h"

namespace saftbus
{

std::shared_ptr<Message> Message::create(const std::vector<int> &fds)
{
	return std::shared_ptr<Message>(new Message(fds));;
}


Message::Message(const std::vector<int> &fds)
{
	// _fd_list = Gio::UnixFDList::create();
	// for(auto fd: fds) {
	// 	_fd_list->append(fd);
	// }
	_fd_list = fds;
}

std::vector<int> &Message::get_fd_list()
{
	return _fd_list;
}

// GUnixFDList* Message::gobj()
// {
// 	return _fd_list->gobj();
// }

InterfaceVTable::InterfaceVTable ( 	
									const std::string &introspection_xml,
									const SlotInterfaceMethodCall&  slot_method_call,
									const SlotInterfaceGetProperty&  slot_get_property,
									const SlotInterfaceSetProperty&  slot_set_property
									)
	: _introspection_xml(introspection_xml)
	, get_property(slot_get_property)
	, set_property(slot_set_property)
	, method_call(slot_method_call)
	
{
	if (_debug_level > 5) std::cerr << "InterfaceVTable::InterfaceVTable() called" << std::endl;
}

InterfaceInfo::InterfaceInfo(const std::string &interface_name, const std::string &xml)
	: _interface_name(interface_name)
	, _xml(xml)
{}
const std::string &InterfaceInfo::get_interface_name()
{
	return _interface_name;
}
const std::string &InterfaceInfo::get_introspection_xml()
{
	return _xml;
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


void MethodInvocation::return_value	(const Serial& parameters)
{
	if (_debug_level > 5) std::cerr << "MethodInvocation::return_value()" << std::endl;
	//if (_debug_level > 5) std::cerr << "   parameters  = " << parameters.print() << std::endl;
	_parameters = parameters;
	//if (_debug_level > 5) std::cerr << "   _parameters = " << _parameters.print() << std::endl;
}
void MethodInvocation::return_error	(const saftbus::Error& error)
{
	_has_error = true;
	_error = error;
}

std::shared_ptr<saftbus::Message>	MethodInvocation::get_message()
{
	std::shared_ptr<Message> msg = Message::create(_fds);
	return msg;
}


Serial& MethodInvocation::get_return_value()
{
	if (_debug_level > 5) std::cerr << "MethodInvocation::get_return_value()" << std::endl;
//	if (_debug_level > 5) std::cerr << "   _parameters = " << _parameters.print() << std::endl;
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




NodeInfo::NodeInfo(const std::string &interface_name, const std::string &xml)
	: _interface_info(new InterfaceInfo(interface_name, xml))
{}

std::shared_ptr<NodeInfo> NodeInfo::create_for_xml (const std::string&  xml_data)
{
	// This function only extracts the interface name from the xml interface description
	// The rest is not needed by saftbus.
	std::string::size_type pos_begin = xml_data.find("interface name=\'");
	if (pos_begin != xml_data.npos) {
		std::string::size_type pos_end = xml_data.find("\'",pos_begin+16u);
		std::string interface_name = xml_data.substr(pos_begin+16u,pos_end-pos_begin-16u);
		return std::shared_ptr<NodeInfo>(new NodeInfo(interface_name, xml_data));
	}
	return std::shared_ptr<NodeInfo>();
}
std::shared_ptr<InterfaceInfo> NodeInfo::lookup_interface ()
{
	if (_debug_level > 5) std::cerr << "NodeInfo::lookup_interface () called" << std::endl;
	return _interface_info;
}

}
