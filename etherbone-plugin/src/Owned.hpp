#ifndef EB_PLUGIN_OWNED_HPP_
#define EB_PLUGIN_OWNED_HPP_

#include <saftbus/service.hpp>

namespace eb_plugin {

	class Owned
	{
	public:
		Owned(saftbus::Container *container);
		virtual ~Owned() = default;

		// @saftbus-export
		void Disown();
		// @saftbus-export
		void Own();
		// @saftbus-export
		void ownerOnly() const;
	private:
		saftbus::Container *cont;
	};

}

#endif