#include "Error.h"


namespace G10
{

namespace BDus
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

}
