#ifndef SAFTBUS_ERROR_H_
#define SAFTBUS_ERROR_H_

#include <giomm.h>

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
		Error(Type type, const Glib::ustring &msg);
		Error(const Error& error);

		Glib::ustring what() const;
		Type type() const;

	private:
		Glib::ustring msg_;
		Type type_;
	};

}


#endif
