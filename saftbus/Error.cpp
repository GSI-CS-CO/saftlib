#include "Error.h"


namespace saftbus
{

	Error::Error()
		: msg_("no error")
		, type_(Error::NO_ERROR)
	{
	}

	Error::Error(Type type, const Glib::ustring &msg)
		: msg_(msg)
		, type_(type)
	{
	}

	Error::Error(const Error& error)
		: msg_(error.msg_)
		, type_(error.type_)
	{
	}


	Glib::ustring Error::what() const
	{
		return msg_;
	}


}

