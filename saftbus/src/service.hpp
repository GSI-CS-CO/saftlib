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

#ifndef SAFTBUS_SERVICE_HPP_
#define SAFTBUS_SERVICE_HPP_

#include "saftbus.hpp"
#include "server.hpp"

// for the SaftbusInfo type
// @saftbus-export
#include "client.hpp" 

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace saftbus {

	/// @brief base class of all saftbus Services
	///
	/// Any Service object maintained by saftbus Container must be derived from saftbus::Service.
	/// In theory, it is possible to write functional Service classes (i.e. derived from this class)
	/// By hand, but there are a number of constraints to fulfill for derived functions to work 
	/// properly in a saftbus::Container. Therefore, derived classes are usually generated from a 
	/// "driver class" using the tool saftbus-gen, which is part of this software package.
	///
	/// For example, if a class named "DriverX" is declared in file driverX.hpp and has 
	/// at least one saftbus tag (// @saftbus-export) on one of its functions or signals, 
	/// a class DriverX_Service can be generated in file driverX_Service.hpp/.cpp by calling
	///
	///     saftbus-gen driverX.hpp
	///
	class Service {
		struct Impl; std::unique_ptr<Impl> d;
		friend class Container;
		friend class Container_Service;
		friend bool operator==(std::pair<const unsigned int, std::unique_ptr<saftbus::Service> > &p, const int fd);
	public:
		/// @brief construct a Service that can be inserted into a saftbus::Container
		///
		/// @param interface_names a list of interfaces that are implemented by the Service. 
		///        Interfaces are usually the name of the driver class and all its base classes.
		/// @param destruction_callback is called whenever a Service is removed from saftbus::Container. 
		///        It allows the driver class to execute some cleanup code in case its related Service object 
		///        is removed from a saftbus::Container
		/// @param destroy_if_owner_quits is true by default, but can be set to false if for some reason the destructible owned
		///        service should not be destroyed when its owner quits.
		Service(const std::vector<std::string> &interface_names, std::function<void()> destruction_callback = std::function<void()>(), bool destoy_if_owner_quits = true);

		/// @brief obtain a lookup table for the interface names. 
		///
		/// The Service class assigns an integer number to each
		/// of its interface names. The number is used in remote function calls to address the correct interface.
		/// This function generates a lookup table from name to assigned number.
		/// Proxy objects of this service use this number to refer to an interface when executing a remote function call
		/// over saftbus. This function is meant to be called by derived Classes generated with saftbus-gen.
		/// 
		/// @param interface_names a list of all interface for which the assigned number is needed.
		/// @param interface_name2no_map the lookup table with the correct integer number assigned to each of the names in interface_names.
		/// @return true if all requested interface names are implemented by the Service calss, false otherwise.
		bool get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map);

		/// @brief execute one of the functions in one of the interfaces of the derived class.
		///
		/// This function extracts interface number and function number from the received data, and calls the pure virtual function call 
		/// that is implemented in the base class.
		/// 
		/// @param client_fd the file descriptor (i.e. the unique ID) of the client that initiated the remote function call.
		/// @param received serialized data containing the arguments of the function call.
		/// @param send a serializer that takes the return values from the function call. 
		///        It will be send back to the client that initiated the remote function call.
		void call(int client_fd, Deserializer &received, Serializer &send);
		virtual ~Service();
	protected:
		/// @brief execute one of the functions in one of the interfaces of the derived class.
		///
		/// Based on two numbers (interface_no and function_no), this function must extract the correct 
		/// types out of received data, do something with it, put the resulting data into the send serializer.
		/// This works only, if the Proxy object and the Service object agree on interface_no, function_no, and 
		/// the expected types and number of parameters and return values.
		/// Therfore, saftbus-gen always generates pairs of classes for each driver class: DriverX_Service and DriverX_Proxy.
		/// 
		/// @param interface_no identifies the interface name. The implementation of call must ensure that the inteface numbers are 
		///                     consistent with the interface_name2no_map lookup table that is generated in get_interface_name2no_map.
		/// @param function_no identifies the function name. 
		/// @param client_fd the file descriptor (i.e. the unique ID) of the client that initiated the remote function call.
		/// @param received serialized data containing the arguments of the function call.
		/// @param send a serializer that takes the return values from the function call. 
		///        It will be send back to the client that initiated the remote function call.
		virtual void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) = 0;


		/// @brief Send some serialized data to all clients (i.e. the SignalGroups connected to this Service).
		///
		/// @param send the serialized data. The first three values in send must be
		///         - the object id (type int) of the Service object in the saftbus::Container
		///         - the interface number (type int) of the interface that sends the signal
		///         - the signal number (type int) of the signal being sent.
		void emit(Serializer &send);

		// @brief get the object id of this Service object in a saftbus::Container
		// @return the object id of this Service object in a saftbus::Container
		int get_object_id();

		// @brief get the object path of this Service object in a saftbus::Container
		// @return the object path of this Service object in a saftbus::Container
		std::string &get_object_path();

		// @brief get all interface names implemented by this Service object
		// @return all interface names implemented by this Service object
		std::vector<std::string> &get_interface_names();

		// @brief get a pointer to the saftbus::Container in which this Service object was inserted
		// @return a pointer to the saftbus::Container in which this Service object was inserted
		Container *get_container();
	};



	/// @brief A Container of Service objects.
	///
	/// Classes derived from Service can be stored here. One instance of Container is hold by 
	/// the Connection object and all Service objects are available for remote Proxy objects to 
	/// register and execute function calls through the Connection.
	class Container {
		// @saftbus-default-object-path /saftbus
		struct Impl; std::unique_ptr<Impl> d;
		friend class Container_Service;
	public:
		
		/// @brief create a Container for saftbus::Service objects
		///
		/// @param connection The Connection object that owns the Container
		Container(ServerConnection *connection);
		~Container();
		/// @brief Insert a Service object and return the saftbus_object_id for this object
		/// @param object_path the object path under which the Service object is available to Proxy objects.
		/// @param service A Service object
		/// @return 0 in case the object_path is already used by another Service object. 
		///         The object_id if the Service object was successfully inserted into the Container
		unsigned create_object(const std::string &object_path, std::unique_ptr<Service> service);

		Service* get_object(const std::string &object_path);


		/// @brief call a Service identified by the saftbus_object_id
		/// @param saftbus_object_id identifies the service object
		/// @param client_fd the file descriptor to the calling client
		/// @param received data that came from the client and is deserialized into function arguments
		/// @param send     serialized return values that will be sent back to the client
		/// @return false if the saftbus_object_id is unknown
		bool call_service(unsigned saftbus_object_id, int client_fd, Deserializer &received, Serializer &send);
		void remove_signal_fd(int fd);

		/// @brief iterate all owned services and remove the ones previously owned by client with this fd
		/// @param fd the file descriptor that signaled a hung-up condition
		void client_hung_up(int fd);


		/// @brief some functions for ownership management. They can only be called whenever a client request is handled.
		int get_calling_client_id() const;
		void set_owner(Service *);
		void active_service_set_owner();
		int  active_service_get_owner() const;
		void active_service_release_owner();
		void active_service_owner_only() const;
		bool active_service_has_destruction_callback() const;
		void active_service_remove();


		/// @brief erase all objects in a safe manner
		///
		/// Children are erased before parents and younger objects are erased before older ones.
		/// Children and parents are identified by the object path: e.g. /grandparent/parent/child
		void clear();

		// return saftbus_object_id if the object_path was found and all requested interfaces are implemented
		// return 0 if object_path was not found
		// return -1 if object_path was found but not all requested interfaces are implemented by the object
		// @saftbus-export 
		int register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string,int> &interface_name2no_map, int client_fd, int signal_group_fd);
		// @saftbus-export
		void unregister_proxy(unsigned saftbus_object_id, int client_fd, int signal_group_fd);
		// @saftbus-export
		bool load_plugin(const std::string &so_filename, const std::vector<std::string> &args = std::vector<std::string>());
		// @saftbus-export
		bool unload_plugin(const std::string &so_filename, const std::vector<std::string> &args = std::vector<std::string>());

		/// @brief remove an object
		/// @param object_path the object path of the service object to be removed
		// @saftbus-export
		bool remove_object(const std::string &object_path);
		// @saftbus-export
		void quit();

		// @saftbus-export
		SaftbusInfo get_status();
	};

	/// @brief created by saftbus-gen from class Container and copied here
	class Container_Service : public saftbus::Service {
		Container* d;
		static std::vector<std::string> gen_interface_names();
	public:
		typedef Container DriverType;
		Container_Service(Container* instance, std::function<void()> destruction_callback = std::function<void()>() );
		Container_Service();
		~Container_Service();
		void call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send);
	};

}

#endif
