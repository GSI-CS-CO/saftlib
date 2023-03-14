/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#include "Owned.hpp"

#include <saftbus/error.hpp>

#include <sstream>


namespace saftlib {

	Owned::Owned(saftbus::Container *container) : cont(container), service(nullptr)
	{ 
	}

	Owned::~Owned() {
		Destroyed();
	}

	void Owned::set_service(saftbus::Service *serv) {
		service = serv;
	}
	void Owned::release_service() {
		service = nullptr;
	}

	void Owned::Disown() {
		if (cont && service) {
			if (service->is_owned() && cont->get_calling_client_id() != service->get_owner()) {
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, "You are not my Owner");
			}
			service->release_owner();
		}
	}
	void Owned::Own() {
		ownerOnly();
		service->set_owner(cont->get_calling_client_id());
	}
	void Owned::ownerOnly() const {
		if (cont && service) {
			if (service->is_owned() && cont->get_calling_client_id() != service->get_owner()) {
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, "You are not my Owner");
			}
		}
	}

	std::string Owned::getOwner() const {
		if (cont && service) {
			if (service->get_owner() != -1) {
				std::ostringstream owner_str;
				owner_str << service->get_owner();
				return owner_str.str();
			}
		}
		return "";
	}

	bool Owned::getDestructible() const {
		if (cont && service) {
			return service->has_destruction_callback();
		}
		return false;
	}

	void Owned::Destroy() {
		ownerOnly();
		if (cont && service) {
			cont->destroy_service(service);
		}
	}
 

}