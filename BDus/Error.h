#ifndef G10_BDUS_ERROR_H_
#define G10_BDUS_ERROR_H_

#include <giomm.h>

namespace G10
{

namespace BDus
{

	class Error
	{
	public:
		enum Type
		{
			INVALID_ARGS,
			UNKNOWN_METHOD,
			IO_ERROR,
			ACCESS_DENIED,
			FAILED,
		};

		Error(Type type, const Glib::ustring &msg);

		Glib::ustring what() const;

	private:
		Glib::ustring msg_;
		Type type_;
	};

}

}

#endif
