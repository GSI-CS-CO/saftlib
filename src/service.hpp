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
	/// In theory, it is possible to write funcitonal Service classes (i.e. derived from this class)
	/// By hand, but there are a number of constraints to fullfill for derived functions to work 
	/// properly in a saftbus::Container. Therefore, derived classes are usually generated from a 
	/// "driver class" using the tool saftbus-gen, which is part of this software package.
	///
	/// For example, if a class named "DriverX" is declared in file driverX.hpp and has 
	/// at least one saftbus tag on one of its functions or signals, 
	/// a class DriverX_Service can be generated in file driverX_Service.hpp/.cpp by calling
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
		Service(const std::vector<std::string> &interface_names, std::function<void()> destruction_callback = std::function<void()>() );

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
		/// @param function_no indentifies the function name. 
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
		/// @brief Insert a Service object and return the saftlib_object_id for this object
		/// @param object_path the object path under which the Service object is available to Proxy objects.
		/// @param service A Service object
		/// @return 0 in case the object_path is already used by another Service object. 
		///         The object_id if the Service object was successfully inserted into the Container
		unsigned create_object(const std::string &object_path, std::unique_ptr<Service> service);


		// call a Service identified by the saftlib_object_id
		// return false if the saftlib_object_id is unknown
		bool call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send);
		void remove_signal_fd(int fd);

		// iterate all owend services and remove the ones previously owned by client with this fd
		void client_hung_up(int fd);


		// these can be called whenever a client request ist handled
		int get_calling_client_id() const;
		void set_owner(Service *);
		void active_service_set_owner();
		int  active_service_get_owner() const;
		void active_service_release_owner();
		void active_service_owner_only() const;
		bool active_service_has_destruction_callback() const;
		void active_service_remove();


		void clear();

		// return saftlib_object_id if the object_path was found and all requested interfaces are implemented
		// return 0 if object_path was not found
		// return -1 if object_path was found but not all requested interfaces are implmented by the object
		// @saftbus-export 
		int register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string,int> &interface_name2no_map, int client_fd, int signal_group_fd);
		// @saftbus-export
		void unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd);
		// @saftbus-export
		bool load_plugin(const std::string &so_filename, const std::vector<std::string> &args = std::vector<std::string>());

		/// @brief remove an object
		/// @param object_path the object path of the service object to be removed
		// @saftbus-export
		bool remove_object(const std::string &object_path);
		// @saftbus-export
		void quit();

		// @saftbus-export
		SaftbusInfo get_status();
	};

	// created by saftbus-gen from class Container and copied here
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


	// class ServerConnection;
	// // A Service to access the Container of Services
	// // mainly Proxy (de-)registration 
	// class Container_Service : public Service {
	// 	struct Impl; std::unique_ptr<Impl> d;
	// public:
	// 	Container_Service(Container *container);
	// 	~Container_Service();
	// 	void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send);
	// };

}

#endif
