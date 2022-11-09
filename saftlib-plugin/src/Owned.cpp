#include "Owned.hpp"


namespace saftlib {

	Owned::Owned(saftbus::Container *container) : cont(container) 
	{ }

	Owned::~Owned() {
		// if (Destroyed) {
		// 	Destroyed();
		// }
	}

	void Owned::Disown() {
		if (cont) {
			cont->active_service_release_owner();
		}
	}
	void Owned::Own() {
		if (cont) {
			cont->active_service_set_owner();
		}
	}
	void Owned::ownerOnly() const {
		if (cont) {
			cont->active_service_owner_only();
		}
	}

	std::string Owned::getOwner() const {
		if (cont) {
			int owner;
			if ((owner=cont->active_service_get_owner()) != -1) {
				std::ostringstream owner_str;
				owner_str << owner;
				return owner_str.str();
			}
		}
		return "";
	}

	bool Owned::getDestructible() const {
		if (cont) {
			return cont->active_service_has_destruction_callback();
		}
		return false;
	}

	void Owned::Destroy() {
		ownerOnly();
		if (cont) {
			cont->active_service_remove();
		}
		// if (Destroyed) {
		// 	Destroyed();
		// }
	}
 

}