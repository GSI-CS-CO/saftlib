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

#include "service.hpp"

#include "plugins.hpp"
#include "loop.hpp"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include <unistd.h>

namespace saftbus {

	struct Service::Impl {
	};

	struct Container::Impl {
	};

	// struct Container_Service::Impl {
	// };


	Service::Service(const std::vector<std::string> &interface_names, std::function<void()> destruction_callback, bool destoy_if_owner_quits) 
	{
		assert(false);
	}
	Service::~Service() = default;

	bool Service::get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map)
	{
		assert(false);
		return false;
	}

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		assert(false);
	}

	void Service::emit(Serializer &send)
	{
		assert(false);
	}

	int Service::get_object_id() 
	{
		assert(false);
		return 0;
	}

	std::string &Service::get_object_path()
	{
		assert(false);
		static std::string str("");
		return str;
	}
	std::vector<std::string> &Service::get_interface_names() {
		assert(false);
		static std::vector<std::string> result;
		return result;
	}




	Container_Service::Container_Service(Container* instance, std::function<void()> destruction_callback )
		: Service(get_interface_names())
	{
		assert(false);
	}
	Container_Service::~Container_Service() = default;

	void Container_Service::call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) {
		assert(false);
	}






	Container::Container(ServerConnection *connection) 
	{
		assert(false);
	}

	Container::~Container() = default;

	unsigned Container::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		assert(false);
		return 0;
	}

	bool Container::remove_object(const std::string &object_path)
	{
		assert(false);
		return false;
	}
	Service* Container::get_object(const std::string &object_path)
	{
		assert(false);
		return nullptr;
	}


	int Container::register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string, int> &interface_name2no_map, int client_fd, int signal_group_fd)
	{
		assert(false);
		return 0;
	}
	void Container::unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd)
	{
		assert(false);
	}


	bool Container::call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send) {
		assert(false);
		return false;
	}

	void Container::remove_signal_fd(int fd)
	{
		assert(false);
	}

	bool Container::load_plugin(const std::string &so_filename, const std::vector<std::string> &args) 
	{
		assert(false);
		return true;
	}

	int Container::get_calling_client_id() const {
		assert(false);
		return -1;
	}
	void Container::set_owner(Service *service) {
		assert(false);
	}
	void Container::active_service_set_owner() {
		assert(false);
	}
	int Container::active_service_get_owner() const {
		assert(false);
	}
	void Container::active_service_release_owner() {
		assert(false);
	}
	void Container::active_service_owner_only() const {
		assert(false);
	}
	bool Container::active_service_has_destruction_callback() const {
		assert(false);
	}
	void Container::active_service_remove() {
		assert(false);
	}

	void Container::clear() {
		assert(false);
	}

	void Container::quit() {
		assert(false);
	}


}
