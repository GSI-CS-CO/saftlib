#include "Error.h"


namespace saftbus
{

	Error::Error(Type type, const Glib::ustring &msg)
		: msg_(msg)
		, type_(type)
	{
	}

	Glib::ustring Error::what() const
	{
		return msg_;
	}


}

