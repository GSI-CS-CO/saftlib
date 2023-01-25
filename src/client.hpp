/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
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

#ifndef SAFTBUS_CLIENT_CONNECTION_HPP_
#define SAFTBUS_CLIENT_CONNECTION_HPP_

#include "saftbus.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

#include <unistd.h>

namespace saftbus {

	class ClientConnection {
		struct Impl; std::unique_ptr<Impl> d;		
	friend class SignalGroup;
	friend class Proxy;
	public:
		ClientConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ClientConnection();
		// send whatever data is in serial buffer to the server
		int send(Serializer &serializer, int timeout_ms = -1); 
		// wait for data to arrive from the server
		int receive(Deserializer &deserializer, int timeout_ms = -1);
	};


	class Proxy;

	class SignalGroup {
		struct Impl; std::unique_ptr<Impl> d;
	friend class Proxy;
	public:
		SignalGroup();
		~SignalGroup();

		int register_proxy(Proxy *proxy);
		void unregister_proxy(Proxy *proxy);

		int get_fd(); // this can be used to hook the SignalGroup into an event loop

		int wait_for_signal(int timeout_ms = -1);
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	};

	class Proxy {
		struct Impl; std::unique_ptr<Impl> d;
	friend class SignalGroup;
	public:
		virtual ~Proxy();
		virtual bool signal_dispatch(int interface_no, int signal_no, Deserializer &signal_content) = 0;
	protected:
		// The Proxy constructor connects the Proxy with a given Service object (identified by the object_path)
		// and connencts the SignalGroup for it.
		// interfaces_names is an array of strings with the interface names in text from.
		// During initialization, the Proxy asks the Service object for the indices that correspont to 
		// the given interface names on this particular service object. The mapping is provided to all 
		// base classes by the method interface_no_from_name.
		Proxy(const std::string &object_path, SignalGroup &signal_group, const std::vector<std::string> &interface_names);
		static ClientConnection& get_connection();
		Serializer&              get_send();
		Deserializer&            get_received();
		int                      get_saftlib_object_id();
		std::mutex&              get_client_socket();


		// this needs to be called by derived classes in order to determine
		// which interface_no they refer to.
		// A specific Proxy only knows the interface name, but not under which number this name can be addressed
		// in the Service object. The Proxy constructor has to get this name->number mapping from the
		// Service object during the initialization phase (the Proxy constructor)
		int interface_no_from_name(const std::string &interface_name); 
	};

	struct SaftbusInfo : public SerDesAble {
		struct ObjectInfo : public SerDesAble {
			unsigned object_id;
			std::string object_path;
			std::vector<std::string> interface_names;
			std::map<int, int> signal_fds_use_count;
			int owner;
			bool has_desctruction_callback;
			void serialize(Serializer &ser) const {
				ser.put(object_id);
				ser.put(object_path);
				ser.put(interface_names);
				ser.put(signal_fds_use_count);
				ser.put(owner);
				ser.put(has_desctruction_callback);
			}
			void deserialize(const Deserializer &des) {
				des.get(object_id);
				des.get(object_path);
				des.get(interface_names);
				des.get(signal_fds_use_count);
				des.get(owner);
				des.get(has_desctruction_callback);
			}
		};
		std::vector<ObjectInfo> object_infos;
		struct ClientInfo : public SerDesAble {
			pid_t process_id;
			int client_fd;
			std::map<int,int> signal_fds;
			void serialize(Serializer &ser) const {
				ser.put(process_id);
				ser.put(client_fd);
				ser.put(signal_fds);
			}
			void deserialize(const Deserializer &des) {
				des.get(process_id);
				des.get(client_fd);
				des.get(signal_fds);
			}
		};
		std::vector<ClientInfo> client_infos;
		void serialize(Serializer &ser) const {
			ser.put(object_infos.size());
			for (auto &object: object_infos) {
				ser.put(object);
			}
			ser.put(client_infos.size());
			for (auto &client: client_infos) {
				ser.put(client);
			}
		}
		void deserialize(const Deserializer &des) {
			size_t size;
			des.get(size);
			object_infos.resize(size);
			for (unsigned i = 0; i < size; ++i) {
				des.get(object_infos[i]);
			}
			des.get(size);
			client_infos.resize(size);
			for (unsigned i = 0; i < size; ++i) {
				des.get(client_infos[i]);
			}
		}
	};


	// created by saftbus-gen from class Container and copied here
	/// @brief A Container of Service objects.
	///
	/// Classes derived from Service can be stored here. One instance of Container is hold by 
	/// the Connection object and all Service objects are available for remote Proxy objects to 
	/// register and execute function calls through the Connection.
	class Container_Proxy : public virtual saftbus::Proxy
	{
		static std::vector<std::string> gen_interface_names();
	public:
		Container_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names = std::vector<std::string>());
		static std::shared_ptr<Container_Proxy> create(const std::string &object_path="/saftbus", saftbus::SignalGroup &signal_group = saftbus::SignalGroup::get_global(), const std::vector<std::string> &interface_names = std::vector<std::string>());
		bool signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content);
		bool load_plugin(const std::string &so_filename, const std::vector<std::string> &plugin_args = std::vector<std::string>());
		bool remove_object(const std::string &object_path);
		void quit();
		SaftbusInfo get_status();
	private:
		int interface_no;

	};


	// class Container_Proxy : public Proxy {
	// public:
	// 	Container_Proxy(const std::string &object_path, SignalGroup &signal_group, std::vector<std::string> interface_names);
	// 	static std::shared_ptr<Container_Proxy> create(SignalGroup &signal_group = SignalGroup::get_global());
	// 	// @saftbus-export
	// 	bool signal_dispatch(int interface_no, 
	// 		                 int signal_no, 
	// 		                 Deserializer& signal_content);
	// 	bool load_plugin(const std::string &so_filename);
	// 	bool remove_object(const std::string &object_path);
	// 	void quit();
	// 	SaftbusInfo get_status();
	// };

}

#endif
