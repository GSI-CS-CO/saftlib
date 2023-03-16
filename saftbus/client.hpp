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

	/// @brief Establish a connection to a running saftbus::ServerConnection using a named socket ("/var/run/saftbus/saftbus" by default)
	/// 
	/// A connection is established by creating a socket pair (using socket_pair function) and send one of the file descriptors to the ServerConnection while keeping the other file descriptor open.
	/// This established an exclusive communication channel between the process with the ClientConnection an the process with the ServerConnection.
	/// The ServerConnection manages multiple such communication channels.
	/// @param socket_name the location of the named socket from the saftbus::ServerConnection
	///
	class ClientConnection {
		struct Impl; std::unique_ptr<Impl> d;		
	friend class SignalGroup;
	friend class Proxy;
	public:
		ClientConnection(const std::string &socket_name = "/var/run/saftbus/saftbus");
		~ClientConnection();
		/// @brief send whatever data is in serial buffer to the server
		///
		/// @param serializer should contain serialized data
		/// @param timeout return after so many milliseconds even if the data could not be sent.
		/// @return 0 in case of timeout, >0 in case of success, -1 in case of error
		int send(Serializer &serializer, int timeout_ms = -1); 
		/// @brief wait for data to arrive from the server
		///
		/// @param deserializer that contains the received buffer after the function returns
		/// @param timeout return after so many milliseconds even if the data could not be sent.
		/// @return 0 in case of timeout, >0 in case of success, -1 in case of error
		int receive(Deserializer &deserializer, int timeout_ms = -1);
	};


	class Proxy;

	/// @brief Manage incoming saftbus signals and distribute them to the connected Proxy objects.
	///
	/// Signals from Services are always sent to an instance of SignalGroup, which manages a file descriptor,
	/// and can wait for incoming signals on this file descriptor (wait_for_signal). If signals arrive, they 
	/// are distributes to all registered Proxy objects with a matching interface (to which the signal is addressed). 
	/// By default, proxy objects are connected to the saftbus::SignalGroup::get_global() signal group.
	/// In some situations (e.g. when independent threads are used), Proxy objects might need their own
	/// channel for signals. A new SignalGroup can be created and passed to the constructor of Proxy objects in order
	/// to assign them to this SignalGroup.
	class SignalGroup {
		struct Impl; std::unique_ptr<Impl> d;
	friend class Proxy;
	public:
		SignalGroup();
		~SignalGroup();

		/// @brief used in the Constructor of Proxy objects to connect themselves to this SignalGroup.
		int register_proxy(Proxy *proxy);
		/// @brief used in the Destructor of Proxy objects to remove themselves from this SignalGroup.
		void unregister_proxy(Proxy *proxy);

		/// @brief Get the file descriptor where the signal are being sent.
		///
		/// This function is intended to be used when saftbus signals need to be integrated into an event loop.
		int get_fd(); // this can be used to hook the SignalGroup into an event loop

		/// @brief Wait for signal to arrive and return either on timeout, or when a number of signals was dispatcht and there are no more signals in the pipe.
		/// 
		/// In most situations, this function should be preferred over wait_for_one_signal.
		/// @param timeout_ms Don't wait longer than so many milliseconds.
		/// @return >0 if a signal was received, 0 if timeout was hit, < 0 in case of failure (e.g. service object was destroyed)
		int wait_for_signal(int timeout_ms = -1);
		/// @brief Wait for a signal to arrive and return either on timeout, or when one signal was dispatched
		///
		/// Use this function only if exactly one signal is to be dispatched. Otherwise use wait_for_signal.
		/// @param timeout_ms Don't wait longer than so many milliseconds.
		/// @return >0 if a signal was received, 0 if timeout was hit, < 0 in case of failure (e.g. service object was destroyed)
		int wait_for_one_signal(int timeout_ms = -1);

		static SignalGroup &get_global();
	};

	/// @brief Base class of all Proxy objects.
	/// 
	/// Any Proxy object must be derived from saftbus::Proxy.
	/// In theory, it is possible to write functional Proxy classes (i.e. derived from this class)
	/// By hand, but there are a number of constraints to fulfill for derived functions to work 
	/// properly. Therefore, Proxy classes are usually generated from a 
	/// "driver class" using saftbus-gen, which is part of this software package.
	///
	/// For example, if a class named "DriverX" is declared in file driverX.hpp and has 
	/// at least one saftbus tag (// @saftbus-export) on one of its functions or signals, 
	/// a class DriverX_Service can be generated in file driverX_Proxy.hpp and driverX_Proxy.cpp by calling
	///
	///     saftbus-gen driverX.hpp
	///
	/// The constructor connects the Proxy with a given Service object (identified by the object_path) and connects the SignalGroup for it.
	/// During initialization, the Proxy asks the Service object for the indices that correspond to 
	/// the given interface names on this particular service object and creates a map.
	/// This map available to all Proxy base classes by the method interface_no_from_name.
	///
	/// @param object_path is the string that identifies the Service object in the saftbus::Container running on the server side.
	/// @param signal_group is the SignalGroup over which this Proxy receives its signals.
	/// @param interfaces_names is an array of strings with the interface names in text from.
	class Proxy {
		struct Impl; std::unique_ptr<Impl> d;
	friend class SignalGroup;
	public:
		virtual ~Proxy();
		/// @brief dispatching function which triggers the actual signals (sigc::signal or std::function) based on the interface_no and signal_no
		/// 
		/// @param interface_no refers to the interface (the mapping can be obtained by interface_no_from_name).
		/// @param signal_no    refers to the signal of a given interface. Signals are numbered by their appearance in the source code.
		virtual bool signal_dispatch(int interface_no, int signal_no, Deserializer &signal_content) = 0;

		/// @brief The signal group to which this proxy belongs.
		/// 
		/// @return a reference to a SignalGroup object
		SignalGroup& get_signal_group();
	protected:
		Proxy(const std::string &object_path, SignalGroup &signal_group, const std::vector<std::string> &interface_names);
		/// @brief Get the client connection. Open the connection if that didn't happen before.
		/// @return reference to the saftbus::ClientConnection object (there is one per process).
		static ClientConnection& get_connection();
		/// @brief Get the serializer that can be used to send data to the Service object.
		/// @return a reference to saftbus::Serializer.
		Serializer&              get_send();
		/// @brief Get the deserializer that can be used to receive data from the Service object.
		/// @return a reference to saftbus::Deserializer.
		Deserializer&            get_received();
		/// @brief The id that was assigned to the Service object of this Proxy
		/// @return the saftbus object id.
		int                      get_saftbus_object_id();
		/// @brief the client socket is a shared resource, it should be locked before using it
		/// @return the socket to lock before using the client socket
		std::mutex&              get_client_socket_mutex();


		/// @brief needs to be called by derived classes in order to determine which interface_no they refer to.
		///
		/// Proxy only knows the interface name, but not under which number this name can be addressed
		/// in the Service object. The Proxy constructor has to get this name->number mapping from the
		/// Service object during the initialization phase (the derived Proxy constructor)
		int interface_no_from_name(const std::string &interface_name); 
	};

	/// @brief contains all information about the status of a saftbus server.
	struct SaftbusInfo : public SerDesAble {
		/// @brief contains all information about a service object
		struct ObjectInfo : public SerDesAble {
			unsigned object_id;
			std::string object_path;
			std::vector<std::string> interface_names;
			std::map<int, int> signal_fds_use_count;
			int owner;
			bool has_destruction_callback;
			bool destroy_if_owner_quits;
			/// @brief custom serializer
			void serialize(Serializer &ser) const {
				ser.put(object_id);
				ser.put(object_path);
				ser.put(interface_names);
				ser.put(signal_fds_use_count);
				ser.put(owner);
				ser.put(has_destruction_callback);
				ser.put(destroy_if_owner_quits);
			}
			/// @brief custom deserializer
			void deserialize(const Deserializer &des) {
				des.get(object_id);
				des.get(object_path);
				des.get(interface_names);
				des.get(signal_fds_use_count);
				des.get(owner);
				des.get(has_destruction_callback);
				des.get(destroy_if_owner_quits);
			}
		};
		std::vector<ObjectInfo> object_infos;
		/// @brief contains all information about a saftbus client connected to a saftbus server
		struct ClientInfo : public SerDesAble {
			pid_t process_id;
			int client_fd;
			std::map<int,int> signal_fds;
			/// @brief custom serializer
			void serialize(Serializer &ser) const {
				ser.put(process_id);
				ser.put(client_fd);
				ser.put(signal_fds);
			}
			/// @brief custom deserializer
			void deserialize(const Deserializer &des) {
				des.get(process_id);
				des.get(client_fd);
				des.get(signal_fds);
			}
		};
		std::vector<ClientInfo> client_infos;
		std::vector<std::string> active_plugins;
		std::map<std::string, std::string> additional_info;
		/// @brief custom serializer
		void serialize(Serializer &ser) const {
			ser.put(object_infos.size());
			for (auto &object: object_infos) {
				ser.put(object);
			}
			ser.put(client_infos.size());
			for (auto &client: client_infos) {
				ser.put(client);
			}
			ser.put(active_plugins);
			ser.put(additional_info);
		}
		/// @brief custom deserializer
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
			des.get(active_plugins);
			des.get(additional_info);
		}
	};


	/// @brief A Container of Service objects.
	///
	/// Classes derived from Service can be stored here. One instance of Container is hold by 
	/// the Connection object and all Service objects are available for remote Proxy objects to 
	/// register and execute function calls through the Connection.
	///
	/// This was created by saftbus-gen from the Container class and manually copied to this place.
	/// The Container class is special and the Proxy code is slightly different from all other Proxy classes.
	class Container_Proxy : public virtual saftbus::Proxy
	{
		static std::vector<std::string> gen_interface_names();
	public:
		Container_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names = std::vector<std::string>());
		static std::shared_ptr<Container_Proxy> create(const std::string &object_path="/saftbus", saftbus::SignalGroup &signal_group = saftbus::SignalGroup::get_global(), const std::vector<std::string> &interface_names = std::vector<std::string>());
		bool signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content);
		bool load_plugin(const std::string &so_filename, const std::vector<std::string> &plugin_args = std::vector<std::string>());
		bool unload_plugin(const std::string &so_filename, const std::vector<std::string> &plugin_args = std::vector<std::string>());
		bool remove_object(const std::string &object_path);
		void quit();
		SaftbusInfo get_status();
	private:
		int interface_no;

	};
}

#endif
