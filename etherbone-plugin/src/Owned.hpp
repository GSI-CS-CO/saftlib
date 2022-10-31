#ifndef EB_PLUGIN_OWNED_HPP_
#define EB_PLUGIN_OWNED_HPP_

#include <saftbus/service.hpp>

namespace eb_plugin {
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
		virtual ~Owned() = default;

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

	protected:
		/// @brief Throw an exception if the caller is not the owner
		void ownerOnly() const;
	private:
		saftbus::Container *cont;
	};

}

#endif