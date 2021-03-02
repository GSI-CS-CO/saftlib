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
		: enabled(false)
		, flush_after_log(flush_often)
		, file(filename.c_str())
	{
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
		std::ostringstream timestamp_out;
		timestamp_out << now.tv_sec << "." << now.tv_nsec << " | ";
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




	FCLogger::FCLogger(std::string n, int size) 
		: name(n)
		, buffer(size,LogEntry())
		, full(false)
		, missing(0)
		, idx(0) 
	{}

	void FCLogger::log(const char *file, int line, const char* func, const char* what, int who, const char *text, uint64_t param) {
		clock_gettime(CLOCK_REALTIME, &buffer[idx].t);
		buffer[idx].file  = file;
		buffer[idx].line  = line;
		buffer[idx].func  = func;
		buffer[idx].what  = what;
		buffer[idx].who   = who;
		buffer[idx].param = param;
		strncpy(buffer[idx].text, text, 32);
		buffer[idx].text[31] = '\0';
		missing += full;      // count up missing entries if full is true
		++idx;                // incerment
		idx %= buffer.size(); // warp around
		full |= !idx;         // set full to true when idx wraps around the first time
	}
	void FCLogger::log_ts(struct timespec ts, const char *file, int line, const char* func, const char* what, int who, const char *text, uint64_t param) {
		buffer[idx].t     = ts;
		buffer[idx].file  = file;
		buffer[idx].line  = line;
		buffer[idx].func  = func;
		buffer[idx].what  = what;
		buffer[idx].who   = who;
		buffer[idx].param = param;
		strncpy(buffer[idx].text, text, 32);
		buffer[idx].text[31] = '\0';
		missing += full;      // count up missing entries if full is true
		++idx;                // incerment
		idx %= buffer.size(); // warp around
		full |= !idx;         // set full to true when idx wraps around the first time
	}

	void FCLogger::dumpline(std::ofstream &out, int idx) {
		out << buffer[idx].t.tv_sec << "." << std::setw(9) << std::setfill('0') << buffer[idx].t.tv_nsec << " " 
			<< buffer[idx].who << " "
		    << buffer[idx].file << ":" 
		    << buffer[idx].line << ":"
		    << buffer[idx].func << " "
		    << buffer[idx].what;
		    if (buffer[idx].param&0x80000000) {
		    	out << "->" << dict[buffer[idx].param&0x7fffffff] << "::";
		    } else {
		    	out << ":" << buffer[idx].param << " ";
		    }
		    out << buffer[idx].text << "\n";
	}

	void FCLogger::dump() {
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		std::ostringstream filename;
		filename << "/tmp/" << name << "_" << now.tv_sec << "." << std::setw(9) << std::setfill('0') << now.tv_nsec
		     << "_" << ".fc_log";
		std::ofstream out(filename.str().c_str());
		if (missing) {
			out << "# " << missing << " missing entries\n";
		}
		if (full) {
			for (int i = idx; i < buffer.size(); ++i) {
				dumpline(out, i);
			}
		}
		for (int i = 0; i < idx; ++i) {
			dumpline(out, i);
		}
		full    = false;
		missing = 0;
		idx     = 0;
	}


}

saftbus::FCLogger fc_logger("saftd",50);