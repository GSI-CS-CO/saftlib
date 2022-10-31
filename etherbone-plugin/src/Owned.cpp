#include "Owned.hpp"


namespace eb_plugin {

	Owned::Owned(saftbus::Container *container) : cont(container) 
	{ }

	void Owned::Disown() {
		if (cont) {
			cont->release_owner();
		}
	}
	void Owned::Own() {
		if (cont) {
			cont->set_owner();
		}
	}
	void Owned::ownerOnly() const {
		if (cont) {
			cont->owner_only();
		}
	}

	std::string Owned::getOwner() const {
		if (cont) {
			int owner;
			if ((owner=cont->get_owner()) != -1) {
				std::ostringstream owner_str;
				owner_str << owner;
				return owner_str.str();
			}
		}
		return "";
	}
}