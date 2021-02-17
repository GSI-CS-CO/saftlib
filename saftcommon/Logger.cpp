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
#include "Logger.h"

#include <iostream>
#include <time.h>

namespace saftbus 
{

	Logger::Logger(const std::string &filename, bool flush_often) 
		: enabled(true)
		, flush_after_log(flush_often)
		, file(filename.c_str())
	{
		clock_gettime(CLOCK_MONOTONIC, &last);
		newMsg(0).add("logger up and running").log();
	}

	void Logger::disable() {
		enabled = false;
	}

	void Logger::enable() {
		enabled = true;
	}

	Logger& Logger::newMsg(int severity) {
		if (enabled) {
			msg.str("");
			msg << std::setw(2)  << severity << "  ";
			msg << std::setw(25) << getTimeTag() << ": ";
		}
		return *this;
	}
	Logger& Logger::add(const std::string &content) {
		return add(content.c_str());
	}	

	void Logger::log() {
		if (enabled) {
			//std::cerr << "\n";
			file << msg.str() << "\n";
			msg.str("");
			if (flush_after_log) {
				file.flush();
			}
		}
	}

	std::string Logger::getTimeTag()
	{
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		double delta = (1.0e6*now.tv_sec   + 1.0e-3*now.tv_nsec) 
		         - (1.0e6*last.tv_sec   + 1.0e-3*last.tv_nsec);
		last = now;
		std::ostringstream timestamp_out;
		timestamp_out << delta << "us:   " << now.tv_sec << "." << now.tv_nsec << " | ";
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char buffer[80];
		strftime(buffer,80,"%c",timeinfo);
		std::string timestring(buffer);
		for (char ch: timestring) if (ch == ':') ch = '.';
		return timestamp_out.str() + timestring;
	}


}
