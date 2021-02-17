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
#ifndef SAFTBUS_LOGGER_H_
#define SAFTBUS_LOGGER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sigc++/sigc++.h>

namespace saftbus 
{

class Logger 
{
public: 
	Logger(const std::string &filename, bool flush_often = true);

	void enable();
	void disable();

	Logger& newMsg(int severity);
	template<class T> 
	Logger& add(const T &content) {
		if (enabled) {
			msg << content;
		}
		return *this;
	}
	Logger& add(const std::string &content);
	void log();

private:

	bool enabled;

	std::string getTimeTag();

	bool flush_after_log;

	std::ostringstream msg;
	std::ofstream file;

	struct timespec last;


};


} // namespace saftbus

#endif 
