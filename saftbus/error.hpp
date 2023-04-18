/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#ifndef SAFTBUS_ERROR_H_
#define SAFTBUS_ERROR_H_

#include <stdexcept>
#include <string>

namespace saftbus
{

	class Error : public std::runtime_error
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
		Error(const std::string &msg);
		Error(const Error& error);

		Type type() const;

	private:
		Type type_;
	};

}


#endif
