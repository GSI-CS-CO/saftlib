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

#ifndef saftlib_OWNED_HPP_
#define saftlib_OWNED_HPP_

#include <saftbus/service.hpp>

namespace saftlib {
	/// de.gsi.saftlib.Owned:
	/// @brief An object which can grant exclusive access if used in a saftbus::Container
	///
	/// This interface allows clients to claim ownership of the object.
	/// When the object has no owner, full access is granted to all clients.
	/// When owned, only the owner may execute priveledged methods.
	/// If the saftbus::Service object has a destruction callback and has an Owner, the object will
	/// be automatically Destroyed when the Owner quits.
	class Owned
	{
	public:
		Owned(saftbus::Container *container);
		virtual ~Owned();

		/// @brief This class only works if it has access to a service object. 
		/// Service object are created after Driver class object. In order to create 
		/// a functional Owned object, the service object pointer must be passed to 
		/// using this function;
		void set_service(saftbus::Service *service);

		/// @brief if a service of an Owned object is destroyed, this method should be 
		/// passed as destruction callback (or should be called in the destruction callback)
		void release_service();

		/// @brief Release ownership of the object.
		///
		/// This method may only be invoked by the current owner of the
		/// object. A disowned object may be accessed by all clients
		/// and will persist until it is destryed explicitly.
		///
		// @saftbus-export
		void Disown();


		/// @brief Claim ownership of the object.
		///
		/// This method may only be invoked if the object is unowned.
		///
		// @saftbus-export
		void Own();

		/// @brief The client which owns this object.
		/// @return client which owns this object.
		///
		/// If there is no Owner, the empty string is returned.
		/// Only the owner may access privileged methods on the object.
		/// When the owning client disconnects, ownership will be 
		/// automatically released, and if the object is Destructable,
		/// the object will also be automatically Destroyed.
		// @saftbus-export
		std::string getOwner() const;


		/// @brief Can the object be destroyed.
		/// @return true if the object has a destruction_callback registered
		///
		/// A destructible object represents a temporary allocated resource. 
		/// When the owner quits, the object will be automatically Destoyed.
		/// Some objects are indestructible, representing a physical resource.
		// @saftbus-export
		bool getDestructible() const;


		/// @brief Destroy this object.
		///
		/// This method may only be invoked by the current owner of the
		/// object. However, if the condition has been disowned, it may
		/// be invoked by anyone.		
		// @saftbus-export
		void Destroy();

		/// @brief The object was destroyed.
		// @saftbus-signal
		std::function<void()> Destroyed;

	protected:
		/// @brief Throw an exception if the caller is not the owner
		void ownerOnly() const;
	private:
		saftbus::Container *cont;
		saftbus::Service *service;
		// int owner;

	};

}

#endif