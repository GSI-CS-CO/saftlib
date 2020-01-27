#include "Error.h"


namespace saftbus
{

	Error::Error()
		: msg_("no error")
		, type_(Error::NO_ERROR)
	{
	}

	Error::Error(Type type, const std::string &msg)
		: msg_(msg)
		, type_(type)
	{
	}

	Error::Error(const Error& error)
		: msg_(error.msg_)
		, type_(error.type_)
	{
	}


	std::string Error::what() const
	{
		return msg_;
	}
	Error::Type Error::type() const
	{
		return type_;
	}


}

