#ifndef SAFTBUS_ERROR_H_
#define SAFTBUS_ERROR_H_

#include <sigc++/sigc++.h>
#include <string>

namespace saftbus
{

	class Error
	{
	public:
		enum Type
		{
			NO_ERROR,
			INVALID_ARGS,
			UNKNOWN_METHOD,
			IO_ERROR,
			ACCESS_DENIED,
			FAILED,
		};

		Error();
		Error(Type type, const std::string &msg);
		Error(const Error& error);

		std::string what() const;
		Type type() const;

	private:
		std::string msg_;
		Type type_;
	};

}


#endif
