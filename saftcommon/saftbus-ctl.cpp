// @brief Command line tool to interact with saftbus devices
// @author Michael Reese  <m.reese@gsi.de>
//
// Copyright (C) 2019-2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// A CLI that allows to see saftbus objects, call their methods and get 
// or set their properties
//
//*****************************************************************************
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
//
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <algorithm>
#include "saftbus.h"
#include "core.h"
#include "Interface.h"
#include "Time.h"

#include <unistd.h>

static void show_help(const char *argv0) 
{
	std::cout << "usage: " << argv0 << " [options]" << std::endl;
	std::cout << "   options are:" << std::endl;
	std::cout << "   -h,--help                          " << std::endl;
	std::cout << "   -s,--status                        " << std::endl;
	std::cout << "   --get-properties <interface-name> <object-path>" << std::endl;
	std::cout << "   --get-property <interface-name> <object-path> <property-name> <type-signature>" << std::endl;
	std::cout << "   --set-property <interface-name> <object-path> <property-name> <type-signature> <property-value>" << std::endl;
	std::cout << "   --call <interface-name> <object-path> <method-name> <return-type-signature> <argument-type-signature> <argument1> ..." << std::endl;
	std::cout << "          the type signature for no argument/return value is v" << std::endl;
	std::cout << "          the type signature for all other types matches the DBus specification" << std::endl;
	std::cout << "   --list-methods <interface-name> <object-path>" << std::endl;
	std::cout << "   --introspect <interface-name> <object-path>" << std::endl;
	std::cout << "   --resize-log-buffer <size>                  " << std::endl;
	std::cout << "   --trigger-logdump                  " << std::endl;
	std::cout << "   --enable-signal-timing-stats       " << std::endl;
	std::cout << "   --disable-signal-timing-stats      " << std::endl;
	std::cout << "   --download-signal-timing-stats     " << std::endl;
}

static std::string print_saftbus_object_table(std::shared_ptr<saftbus::ProxyConnection> connection) 
{
	saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_GET_STATE);


	std::map<std::string, std::map<std::string, int> > saftbus_indices; 
	saftbus::read(connection->get_fd(), saftbus_indices);
	std::vector<int> indices;
	std::vector<int> assigned_indices;
	saftbus::read(connection->get_fd(), indices);

	int saftbus_object_id_counter; // log saftbus object creation
	saftbus::read(connection->get_fd(), saftbus_object_id_counter);
	int saftbus_signal_handle_counter; // log signal subscriptions
	saftbus::read(connection->get_fd(), saftbus_signal_handle_counter);

	std::vector<int> sockets_active;
	saftbus::read(connection->get_fd(), sockets_active);

	std::map<int, std::string> socket_owner;
	saftbus::read(connection->get_fd(), socket_owner);

	// 	     // handle    // signal
	//std::map<unsigned, sigc::signal<void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Glib::VariantContainerBase&> > _handle_to_signal_map;
	std::map<unsigned, int> handle_to_signal_map;
	saftbus::read(connection->get_fd(), handle_to_signal_map);
	// for (auto handle_signal: handle_to_signal_map) {
	// 	std::cout << handle_signal.first << " " << handle_signal.second << std::endl;
	// }


	std::map<std::string, std::set<unsigned> > id_handles_map;
	saftbus::read(connection->get_fd(), id_handles_map);


	//std::set<unsigned> erased_handles;
	std::vector<unsigned> erased_handles;
	saftbus::read(connection->get_fd(), erased_handles);



	// store the pipes that go directly to one or many Proxy objects
			// interface_name        // object path
	std::map<std::string, std::map < std::string , std::set< saftbus::SignalFD > > > proxy_pipes;
	saftbus::read(connection->get_fd(), proxy_pipes);

	int _saftbus_id_counter;
	saftbus::read(connection->get_fd(), _saftbus_id_counter);

	std::map<std::string, std::string> owners;
	saftbus::read(connection->get_fd(), owners);

	std::ostringstream oss;


	oss << std::endl;
	oss << "_____________________________________________________________________________________________________________" << std::endl;
	oss << std::endl;

	oss << std::left << std::setw(50) << "object path" 
	          << std::left << std::setw(50) << "interface name{,proxy signal pipe fds}[owner]" 
	          << std::endl;
	oss << "_____________________________________________________________________________________________________________" << std::endl;
	oss << std::endl;

	std::vector<std::pair<std::string, std::pair<std::string, std::string> > > table;

	for (auto saftbus_index: saftbus_indices) {
		for (auto object_path: saftbus_index.second) {
			std::string interface_name = saftbus_index.first;
			std::string obj_path = object_path.first;
			std::string owner;
			for (auto pp: proxy_pipes[interface_name][obj_path]) {
				interface_name.append(",");
				std::ostringstream fdout;
				fdout << pp.fd;
				interface_name.append(fdout.str());
			}
			if (saftbus_index.first == "de.gsi.saftlib.Owned") {
				owner = owners[object_path.first];
				if (owner != "") {
					interface_name.append("[");
					interface_name.append(owner);
					interface_name.append("]");
				}
			}
			table.push_back(std::make_pair(obj_path, std::make_pair(interface_name, owner)));
		}
	}
	std::sort(table.begin(), table.end(), [](std::pair<std::string, std::pair<std::string, std::string> > a,
		                                     std::pair<std::string, std::pair<std::string, std::string> > b) 
	                                        { return a.first < b.first; });
	std::string previous_object_path;
	for (auto line: table) {
		if (previous_object_path != line.first) {
			if (!previous_object_path.empty()) oss << std::endl;
			oss << std::left << std::setw(50) << line.first;
		} else {
			oss << std::left << std::setw(50) << "";
		}
		oss << std::left << std::setw(50) << line.second.first;
		oss << std::endl;
		previous_object_path = line.first;
	}
	oss << "_____________________________________________________________________________________________________________" << std::endl;


	oss << std::endl;
	oss << std::setw(7) << "socket" << std::setw(7) << " owner" << std::endl;
	for (auto owner: socket_owner) {
		oss << std::setw(7) << owner.first << std::setw(7) << owner.second << std::endl;
	}
	return oss.str();
}

template<typename T>
std::string print_serial_value(saftbus::Serial &s) {
	T value;
	s.get(value);
	std::ostringstream oss;
	oss << value;
	return oss.str();
}
template<>
std::string print_serial_value<saftlib::Time>(saftbus::Serial &s) {
	saftlib::Time value;
	s.get(value);
	std::ostringstream oss;
	oss << value.getTAI();
	return oss.str();
}
template<>
std::string print_serial_value<unsigned char>(saftbus::Serial &s) {
	unsigned char value;
	s.get(value);
	std::ostringstream oss;
	oss << static_cast<int>(value);
	return oss.str();
}
template<typename T>
std::string print_serial_vector(saftbus::Serial &s) {
	std::vector<T> values;
	s.get(values);
	std::ostringstream oss;
	for (auto value: values) {
		oss << value << " ";
	}
	return oss.str();
}
template<typename K, typename V>
std::string print_serial_map(saftbus::Serial &s) {
	std::map<K,V> map;
	std::ostringstream oss;
	s.get(map);
	oss <<"{";
	int i = 0;
	for (auto pair: map) {
		if (i++) oss << ",";
		oss << pair.first << ":" << pair.second;
	}
	oss <<"}";
	return oss.str();
}

template<typename K1, typename K2, typename V>
std::string print_serial_map_map(saftbus::Serial &s) {
	std::map<K1, std::map<K2,V> > map;
	std::ostringstream oss;
	s.get(map);
	oss <<"{";
	int i = 0;
	for (auto inner_map: map) {
		if (i++) oss << ",";
		oss << inner_map.first << ":";
		oss << "{";
		int j = 0;
		for (auto pair: inner_map.second) {
			if (j++) oss << ",";
			oss << pair.first << ":" << pair.second;
		}
		oss << "}";
	}
	oss << "}";
	return oss.str();
}
static std::string saftbus_list_methods(std::shared_ptr<saftbus::ProxyConnection> connection,
	                      const std::string& interface_name,
	                      const std::string& object_path) 
{
	std::string introspection_xml = connection->introspect(object_path, interface_name);
	std::ostringstream oss;
	//std::cerr << introspection_xml << std::endl;
	size_t pos_method_end = 0;
	for(int i = 0;;++i) {
		auto pos_method_begin = introspection_xml.find("<method", pos_method_end);
		if (pos_method_begin == std::string::npos) break;
		pos_method_end = introspection_xml.find("/method>", pos_method_begin);

		std::istringstream in(introspection_xml.substr(pos_method_begin, pos_method_end-pos_method_begin));

		std::string method, raw_name;
		in >> method >> raw_name;
		std::string name = raw_name.substr(6, raw_name.find_last_of('\'')-6);
		if (i) oss << std::endl;
//		std::cerr << " >>>> " << raw_name << " " << raw_name[raw_name.find_last_of('\'')+1] << std::endl;
		oss << name;
		if (raw_name[raw_name.find_last_of('\'')+1] != '/')// this method has arguments
		{
			for (;;) {
				std::string token;
				in >> token;
				if (!in) break;
				auto pos1 = token.find('\'');
				auto pos2 = token.find_last_of('\'');
				if (token.substr(0,10) == "direction=") {
					oss << " " << token.substr(pos1+1,pos2-pos1-1);
				} 
				if (token.substr(0,5) == "type=") {
					oss << ":" << token.substr(pos1+1,pos2-pos1-1);
				} 
				if (token.substr(0,5) == "name=") {
					oss << ":" << token.substr(pos1+1,pos2-pos1-1) << " ";
				} 
			}
		} else {
			pos_method_end = pos_method_begin+2; // have to correct the end position in this case because looking for /method as end position was wrong
		}

	}
	oss << std::endl;
	return oss.str();
}

static std::string saftbus_get_property(std::shared_ptr<saftbus::ProxyConnection> connection,
	                      const std::string& interface_name,
	                      const std::string& object_path,
	                      const std::string& property_name,
	                      const std::string& property_type_signature) 
{
	saftbus::Serial params;
	params.put(interface_name);
	params.put(property_name);

	int saftbus_index = connection->get_saftbus_index(object_path, interface_name);
	saftbus::Serial val = connection->call_sync(saftbus_index, object_path, "org.freedesktop.DBus.Properties", "Get", 
	  params, "sender");

	std::ostringstream oss;

	val.get_init();
	if (property_type_signature == "y") { oss << print_serial_value<unsigned char>(val); return oss.str();;}
	if (property_type_signature == "b") { oss << print_serial_value<bool>(val); return oss.str();;}
	if (property_type_signature == "n") { oss << print_serial_value<int16_t>(val); return oss.str();;}
	if (property_type_signature == "q") { oss << print_serial_value<uint16_t>(val); return oss.str();;}
	if (property_type_signature == "i") { oss << print_serial_value<int32_t>(val); return oss.str();;}
	if (property_type_signature == "u") { oss << print_serial_value<uint32_t>(val); return oss.str();;}
	if (property_type_signature == "x") { oss << print_serial_value<int64_t>(val); return oss.str();;}
	if (property_type_signature == "t") { oss << print_serial_value<uint64_t>(val); return oss.str();;}
	if (property_type_signature == "d") { oss << print_serial_value<double>(val); return oss.str();;}
	if (property_type_signature == "h") { oss << print_serial_value<int>(val); return oss.str();;}
	if (property_type_signature == "s") { oss << print_serial_value<std::string>(val); return oss.str();;}

	if (property_type_signature == "ay") { oss << print_serial_vector<unsigned char>(val); return oss.str();;}
	if (property_type_signature == "ab") { oss << print_serial_vector<bool>(val); return oss.str();;}
	if (property_type_signature == "an") { oss << print_serial_vector<int16_t>(val); return oss.str();;}
	if (property_type_signature == "aq") { oss << print_serial_vector<uint16_t>(val); return oss.str();;}
	if (property_type_signature == "ai") { oss << print_serial_vector<int32_t>(val); return oss.str();;}
	if (property_type_signature == "au") { oss << print_serial_vector<uint32_t>(val); return oss.str();;}
	if (property_type_signature == "ax") { oss << print_serial_vector<int64_t>(val); return oss.str();;}
	if (property_type_signature == "at") { oss << print_serial_vector<uint64_t>(val); return oss.str();;}
	if (property_type_signature == "ad") { oss << print_serial_vector<double>(val); return oss.str();;}
	if (property_type_signature == "ah") { oss << print_serial_vector<int>(val); return oss.str();;}
	if (property_type_signature == "as") { oss << print_serial_vector<std::string>(val); return oss.str();;}

	if (property_type_signature == "a{ss}") { oss << print_serial_map<std::string,std::string>(val); return oss.str();;}

	if (property_type_signature == "a{sa{ss}}") { oss << print_serial_map_map<std::string,std::string,std::string>(val); return oss.str();;}

	oss << "unsupported type signature" << std::endl;
	return oss.str();
}

static std::string saftbus_get_properties(std::shared_ptr<saftbus::ProxyConnection> connection,
	                        const std::string &interface_name,
	                        const std::string &object_path) 
{
	std::string introspection_xml = connection->introspect(object_path, interface_name);
	//std::cerr << introspection_xml << std::endl;
	size_t pos_property_end = 0;
	std::ostringstream oss;
	for(;;) {
		auto pos_property_begin = introspection_xml.find("<property", pos_property_end);
		if (pos_property_begin == std::string::npos) break;
		pos_property_end = introspection_xml.find("/>", pos_property_begin);

		std::istringstream in(introspection_xml.substr(pos_property_begin, pos_property_end-pos_property_begin));
		std::string property, name, type, access;

		in >> property >> name >> type >> access;

		name   = name.substr(6, name.find_last_of('\'')-6);
		type   = type.substr(6, type.find_last_of('\'')-6);
		access = access.substr(8, access.find_last_of('\'')-8);

		oss << type << ":" << access << ":" << name << "=" << saftbus_get_property(connection, interface_name, object_path, name, type) << std::endl;
	}
	return oss.str();
}


static std::string saftbus_set_property(std::shared_ptr<saftbus::ProxyConnection> connection,
	                      const std::string& interface_name,
	                      const std::string& object_path,
	                      const std::string& property_name,
	                      const std::string& property_type_signature,
	                      const std::string& value) 
{
	std::ostringstream oss;
	saftbus::Serial params;
	params.put(interface_name);
	params.put(property_name);
	saftbus::Serial property_value;
	std::istringstream value_in(value);
	if (property_type_signature == "y") { unsigned char value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "b") { bool          value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "n") { int16_t       value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "q") { uint16_t      value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "i") { int32_t       value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "u") { uint32_t      value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "x") { int64_t       value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "t") { uint64_t      value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "d") { double        value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "h") { int           value; value_in >> value; property_value.put(value); }
	if (property_type_signature == "s") { std::string   value; value_in >> value; property_value.put(value); }
	params.put(property_value);

	int saftbus_index = connection->get_saftbus_index(object_path, interface_name);
	try {
	saftbus::Serial val = connection->call_sync(saftbus_index, object_path, "org.freedesktop.DBus.Properties", "Set", 
	  params, "sender");
	} catch (const saftbus::Error& error) {
		oss << "Set property failed: " << error.what() << std::endl;
	}
	return oss.str();
} 

static std::string saftbus_method_call (std::shared_ptr<saftbus::ProxyConnection> connection,
	                      const std::string& interface_name,
	                      const std::string& object_path,
	                      const std::string& method_name,
	                      const std::string return_type_signature, 
	                      const std::string& type_signature,
	                      const std::vector<std::string>& arguments)
{
	//std::cerr << "method call " << object_path << " " << interface_name << " " << return_type_signature << " " << type_signature << " " << arguments.size() << std::endl;
	std::ostringstream oss;
	saftbus::Serial args;
	for (unsigned i = 0; i < type_signature.size(); ++i) {
		if (arguments.size() <= i) break;
		std::istringstream value_in(arguments[i]);
		if (type_signature[i] == 'y')      { unsigned char value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'b') { bool          value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'n') { int16_t       value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'q') { uint16_t      value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'i') { int32_t       value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'u') { uint32_t      value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'x') { int64_t       value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 't') { uint64_t      value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'T') { uint64_t      value; value_in >> value; args.put(saftlib::makeTimeTAI(value)); }
		else if (type_signature[i] == 'd') { double        value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'h') { int           value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 's') { std::string   value; value_in >> value; args.put(value); }
		else if (type_signature[i] == 'v') { /* nothing */ }
		else {oss << "unknow type signature for method call " ; return oss.str(); }
	}
	int saftbus_index = connection->get_saftbus_index(object_path, interface_name);
	try {
		saftbus::Serial val = connection->call_sync(saftbus_index, object_path, interface_name, method_name, args, "sender");

		val.get_init();
		if (return_type_signature == "a{sa{ss}}") { oss << print_serial_map_map<std::string,std::string,std::string>(val); }
		else if (return_type_signature == "a{ss}") { oss << print_serial_map<std::string,std::string>(val); }
		else if (return_type_signature == "as") { oss << print_serial_vector<std::string>(val); }
		else if (return_type_signature == "y") { oss << print_serial_value<unsigned char>(val); }
		else if (return_type_signature == "b") { oss << print_serial_value<bool>(val); }
		else if (return_type_signature == "n") { oss << print_serial_value<int16_t>(val); }
		else if (return_type_signature == "q") { oss << print_serial_value<uint16_t>(val); }
		else if (return_type_signature == "i") { oss << print_serial_value<int32_t>(val); }
		else if (return_type_signature == "u") { oss << print_serial_value<uint32_t>(val); }
		else if (return_type_signature == "x") { oss << print_serial_value<int64_t>(val); }
		else if (return_type_signature == "t") { oss << print_serial_value<uint64_t>(val); }
		else if (return_type_signature == "T") { oss << print_serial_value<saftlib::Time>(val); }
		else if (return_type_signature == "d") { oss << print_serial_value<double>(val); }
		else if (return_type_signature == "h") { oss << print_serial_value<int>(val); }
		else if (return_type_signature == "s") { oss << print_serial_value<std::string>(val); }
		else if (return_type_signature == "v") { /*void ... do nothing*/ }
		else if (return_type_signature == "") { /*void ... do nothing*/ }
		else {oss  << "unsupported return type signature" << std::endl; return oss.str(); }
	} catch (const saftbus::Error& error) {
		oss << "Method call failed: " << error.what() << std::endl;
	}
	return oss.str();
} 


int main(int argc, char *argv[])
{
	try {
		//Glib::init();

		bool interactive_mode             = false;
		bool list_mutable_state           = false;
		bool enable_signal_stats          = false;
		bool disable_signal_stats         = false;
		bool save_signal_time_stats       = false;
		bool resize_log_buffer               = false;
		bool trigger_logdump              = false;
		bool get_property                 = false;
		bool set_property                 = false;
		bool call_method                  = false;
		bool introspect                   = false;
		bool get_properties               = false;
		bool list_methods                 = false;

		std::string interface_name;
		std::string object_path;
		std::string property_name;
		std::string type_signature;
		std::string return_type_signature;
		std::string property_value;
		std::string method_name;
		std::vector<std::string> method_arguments;

		int new_logbuffer_size = -1;

		std::string timing_stats_filename = "saftbus_timing.dat";

		if (argc == 1) {
			show_help(argv[0]);
			return 0;
		}

		for (int i = 1; i < argc; ++i) {
			std::string argvi = argv[i];
			if (argvi == "-h" || argvi == "--help") {
				show_help(argv[0]);
				return 0;
			} else if (argvi == "-i" || argvi == "--interactive") {
				interactive_mode = true;
			} else if (argvi == "-s" || argvi == "--status") {
				list_mutable_state = true;
			} else if (argvi == "--download-signal-timing-stats") {
				save_signal_time_stats = true;
			} else if (argvi == "--enable-signal-timing-stats") {
				enable_signal_stats = true;
			} else if (argvi == "--disable-signal-timing-stats") {
				disable_signal_stats = true;
			} else if (argvi == "--resize-log-buffer") {
				resize_log_buffer = true;
				if (argc - i < 1) {
					std::cerr << "expect 4 arguments after --resize-log-buffer" << std::endl;
					return 1;
				} else {
					std::istringstream in(argv[++i]);
					in >> new_logbuffer_size;
					if (!in) {
						std::cerr << "cannot read logbuffer size: " << argv[i] << std::endl;
						return 1;
					}
				}
			} else if (argvi == "--trigger-logdump") {
				trigger_logdump = true;
			} else if (argvi == "--get-property") {
				get_property = true;
				if (argc - i < 4) {
					std::cerr << "expect 4 arguments after --get-property" << std::endl;
					std::cerr << "        interface_name" << std::endl;
					std::cerr << "        object_path" << std::endl;
					std::cerr << "        property_name" << std::endl;
					std::cerr << "        property_type_signature" << std::endl;
				} else {
					interface_name          = argv[++i];
					object_path             = argv[++i];
					property_name           = argv[++i];
					type_signature          = argv[++i];
				}
			} else if (argvi == "--set-property") {
				set_property = true;
				if (argc - i < 5) {
					std::cerr << "expect 5 arguments after --set-property" << std::endl;
					std::cerr << "        interface_name" << std::endl;
					std::cerr << "        object_path" << std::endl;
					std::cerr << "        property_name" << std::endl;
					std::cerr << "        property_type_signature" << std::endl;
				} else {
					interface_name          = argv[++i];
					object_path             = argv[++i];
					property_name           = argv[++i];
					type_signature          = argv[++i];
					property_value          = argv[++i];
				}
			} else if (argvi == "--call") {
				call_method = true;
				if (argc - i <= 5) {
					std::cerr << "expect >= 5 arguments after --call" << std::endl;
					std::cerr << "        interface_name" << std::endl;
					std::cerr << "        object_path" << std::endl;
					std::cerr << "        property_name" << std::endl;
					std::cerr << "        return_type_signature" << std::endl;
					std::cerr << "        arg_type_signature" << std::endl;
					std::cerr << "        [arguments ...]" << std::endl;
				} else {
					interface_name          = argv[++i];
					object_path             = argv[++i];
					method_name             = argv[++i];
					return_type_signature   = argv[++i];
					type_signature          = argv[++i];

					while(i < argc-1) {
						method_arguments.push_back(argv[++i]);
					}
				}
			} else if (argvi == "--introspect" || argvi == "--get-properties" || argvi == "--list-methods") {
				if (argvi == "--introspect") introspect = true;
				if (argvi == "--get-properties") get_properties = true;
				if (argvi == "--list-methods") list_methods = true;
				if (argc - i <= 2) {
					std::cerr << "expect >= 2 arguments after --introspect" << std::endl;
					std::cerr << "        interface_name" << std::endl;
					std::cerr << "        object_path" << std::endl;
				} else {
					interface_name          = argv[++i];
					object_path             = argv[++i];
				}
			} else {
				std::cerr << "unknown argument: " << argvi << std::endl;
				return 1;
			}
		}
		// connect to saft-daemon
		auto connection = std::shared_ptr<saftbus::ProxyConnection>(new saftbus::ProxyConnection);

		if (list_mutable_state) {
			//print_mutable_state(connection);
			std::cout << print_saftbus_object_table(connection) << std::endl;
		}

		if (enable_signal_stats && disable_signal_stats) {
			std::cerr << "you can either disable or enable signal stats," << std::endl;
			std::cerr << " not both at the same time" << std::endl;
			return 1;
		}

		if (enable_signal_stats) {
			std::cout << "enabling signal flight time statistics in saftd" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_ENABLE_STATS);
		}
		if (disable_signal_stats) {
			std::cout << "disabling signal flight time statistics in saftd" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_DISABLE_STATS);
		}

		if (save_signal_time_stats) {
			std::cout << "downloading signal timing statistics from saftd" << std::endl;
			std::map<int, int> signal_flight_times;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_GET_STATS);
			saftbus::read(connection->get_fd(), signal_flight_times);
			std::ofstream statfile("signal_flight_times.dat");
			for (auto entry: signal_flight_times) {
				statfile << entry.first << " " << entry.second << std::endl;
			}
			statfile.close();
		}

		if (resize_log_buffer > 0) {
			std::cout << "resize log buffer" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_ENABLE_LOGGING);
			saftbus::write(connection->get_fd(), new_logbuffer_size);
		}
		if (trigger_logdump) {
			std::cout << "trigger logdump" << std::endl;
			saftbus::write(connection->get_fd(), saftbus::SAFTBUS_CTL_DISABLE_LOGGING);
		}

		if (get_property) {
			std::cout << saftbus_get_property(connection, interface_name, object_path, property_name, type_signature);
		}
		if (get_properties) {
			std::cout << saftbus_get_properties(connection, interface_name, object_path);
		}
		if (set_property) {
			std::cout << saftbus_set_property(connection, interface_name, object_path, property_name, type_signature, property_value);
		}
		if (call_method) {
			try {	
				std::cout << saftbus_method_call(connection, interface_name, object_path, method_name, return_type_signature, type_signature, method_arguments) << std::endl;
			} catch(saftbus::Error &e) {
				std::cerr << "exception retured from method call: " << e.what() << std::endl;
			}
		}
		if (list_methods) {
			std::cout << saftbus_list_methods(connection, interface_name, object_path);
		}
		if (introspect) {
			try {
				std::cout << "introspecting " << object_path << " " << interface_name << std::endl; 
				std::cout << connection->introspect(object_path, interface_name) << std::endl;
			} catch(saftbus::Error &e) {
				std::cerr << "exepction retured from introspection: " << e.what() << std::endl;
			}
		}
		if (interactive_mode) {
			//std::ofstream outputfile("/tmp/saftbus-ctl-output.txt");
			std::ostream &out = std::cout;
			bool stop = false;;
			while (!stop) {
				std::string line;
				std::vector<std::string> cmd;
				out << "saftbus" << connection->get_saftbus_id() << "> " << std::flush;

				for (;;) {
					char c;
					if (read(0, &c, sizeof(c)) != sizeof(c)) {
						stop = true;
						break;
					}
					if (c == '\n') {
						break;
					}
					line.push_back(c);
				}
				// std::getline(std::cin,line);
				// if (!std::cin) break;
				std::istringstream lin(line);
				for (;;) {
					std::string token;
					lin >> token;
					if (!lin) break;
					cmd.push_back(token);
				}
				if (cmd.size() == 0) continue;
				if (cmd[0] == "set-property") {
					if (cmd.size() != 6) {
						out << "error: expecting: " << cmd[0] << " <interface_name> <object_path> <property-name> <type-signture> <value>" << std::endl;
					} else   {
						out << saftbus_set_property(connection, cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]) << std::endl;
					}
				}
				if (cmd[0] == "get-property") {
					if (cmd.size() != 5) {
						out << "error: expecting: " << cmd[0] << " <interface_name> <object_path> <property-name> <type-signture>" << std::endl;
					} else {
						out << saftbus_get_property(connection, cmd[1], cmd[2], cmd[3], cmd[4]) << std::endl;
					}
				}
				if (cmd[0] == "call") {
					if (cmd.size() < 6) {
						out << "error: expecting: " << cmd[0] << " <interface_name> <object_path> <method-name> <return-type-signture> <arg-type-signture> {<args>}" << std::endl;
					} else {
						std::vector<std::string> args;
						for(size_t i = 6; i < cmd.size(); ++i) args.push_back(cmd[i]);
						out << saftbus_method_call(connection, cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], args) << std::endl;
					}
				}			
				if (cmd[0] == "introspect" || cmd[0] == "list-methods" || cmd[0] == "get-properties") {
					if (cmd.size() != 3) {
						out << "error: expecting: " << cmd[0] << " <interface_name> <object_path>" << std::endl;
					} else {
						if (cmd[0] == "introspect") out << connection->introspect(cmd[2], cmd[1]) << std::endl;
						if (cmd[0] == "list-methods") out << saftbus_list_methods(connection, cmd[1], cmd[2]);
						if (cmd[0] == "get-properties") out << saftbus_get_properties(connection, cmd[1], cmd[2]);
					}
				}
				if (cmd[0] == "status") {
					if (cmd.size() != 1) {
						out << "error: expecting no arguments to status" << std::endl;
					} else {
						out << print_saftbus_object_table(connection) << std::endl;
					}
				}
			}
		}

		connection.reset();
	} catch (...) {
		std::cout << "Error:" << std::endl;
		std::cout << "   Failed to connect to saftd. " << std::endl;
		std::cout << "   Either all sockets are busy, or saftd isn't running at all." << std::endl;
	}
	return 0;
}