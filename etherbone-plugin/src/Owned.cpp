#include "Owned.hpp"


namespace eb_plugin {

	Owned::Owned(saftbus::Container *container) : cont(container) 
	{ }

	void Owned::Disown() {
		if (cont) {
			// cont->release_owner();
		}
	}
	void Owned::Own() {
		if (cont) {
			// cont->set_owner();
		}
	}
	void Owned::ownerOnly() const {
		if (cont) {
			// cont->owner_only();
		}
	}
}