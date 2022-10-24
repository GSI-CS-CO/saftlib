/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include "error.hpp"


namespace saftbus
{

	Error::Error()
		: std::runtime_error("no error")
		, type_(Error::NO_ERROR)
	{
	}

	Error::Error(Type type, const std::string &msg)
		: std::runtime_error(msg)
		, type_(type)
	{
	}

	Error::Error(const Error& error)
		: std::runtime_error(error)
		, type_(error.type_)
	{
	}

	Error::Type Error::type() const
	{
		return type_;
	}


}

