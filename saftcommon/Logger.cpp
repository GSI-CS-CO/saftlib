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
#include "GELF.hpp"

#include <iostream>
#include <time.h>
#include <unistd.h>
#include <limits.h>

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




	FCLogger::FCLogger(std::string n, int size, int level, int num) 
		: name(n), message(gelf::Severity::Warning, "nothing")//, saftdlog("/tmp/saftdddd.log")
	{
		char hostn[HOST_NAME_MAX];
		gethostname(hostn, HOST_NAME_MAX);
		hostname = hostn;

		resize(size);
		log_level = level;
		num_dumps = num;
		gelf::initialize();
		if (hostname.substr(0,3)=="scu") {
			gelf::configure("graylog.acc.gsi.de", 12201);
		} else {
			gelf::configure("localhost", 12201);
		}


	}

	void FCLogger::log(const char *file, int line, const char* func, const char* what, int who, const char *text, int64_t dict, int64_t param) {
		if (who >= log_level) return; // loglevel 0 effectively disables logging 
		clock_gettime(CLOCK_REALTIME, &buffer[idx].t);
		buffer[idx].file  = file;
		buffer[idx].line  = line;
		buffer[idx].func  = func;
		buffer[idx].what  = what;
		buffer[idx].who   = who;
		buffer[idx].param = param;
		buffer[idx].dict  = dict;
		//strncpy(buffer[idx].text, text, 32);
		for (int i = 0; i<32; ++i) {
			buffer[idx].text[i] = text[i];
			if (!text[i]) break;
		}
		buffer[idx].text[31] = '\0';
		missing += full;      // count up missing entries if full is true
		++idx;                // incerment
		idx %= buffer.size(); // warp around
		full |= !idx;         // set full to true when idx wraps around the first time
	}
	void FCLogger::log_ts(struct timespec ts, const char *file, int line, const char* func, const char* what, int who, const char *text, int64_t dict, int64_t param) {
		if (who >= log_level) return; // loglevel 0 effectively disables logging 
		buffer[idx].t     = ts;
		buffer[idx].file  = file;
		buffer[idx].line  = line;
		buffer[idx].func  = func;
		buffer[idx].what  = what;
		buffer[idx].who   = who;
		buffer[idx].param = param;
		buffer[idx].dict  = dict;
		for (int i = 0; i<32; ++i) {
			buffer[idx].text[i] = text[i];
			if (!text[i]) break;
		}
		buffer[idx].text[31] = '\0';
		missing += full;      // count up missing entries if full is true
		++idx;                // incerment
		idx %= buffer.size(); // warp around
		full |= !idx;         // set full to true when idx wraps around the first time
	}

	void FCLogger::dumpline(std::ostream &out, int idx) {
		out << std::dec;
		out << buffer[idx].t.tv_sec << "." << std::setw(9) << std::setfill('0') << buffer[idx].t.tv_nsec << " " 
			<< buffer[idx].who << " "
		    << buffer[idx].file << ":" 
		    << buffer[idx].line << ":"
		    << buffer[idx].func << " ";
		    if (strlen(buffer[idx].what)) {
		    	out << buffer[idx].what;
		    }
		    if (buffer[idx].dict >= 0) {
		    	out << ":" << dict[buffer[idx].dict];
		    } 
		    if (strlen(buffer[idx].text)) {
		    	out << ":" << buffer[idx].text;
		    }
		    if (buffer[idx].param >= 0) {
		    	out << ":" << buffer[idx].param;
		    } 
		    out << "\n";
	}


	void FCLogger::gelf_post(bool force) {
		if (message_lines == 100 || force) {
			message.fullMessage(fullmsg.str());
			fullmsg.str("");
			gelf::post(message.build());
			// saftdlog << message.build().getMessage() << std::endl << std::endl;
			message_lines = 0;
		}
	}

	void FCLogger::dump_gelf() {
			std::cerr << "dump_gelf" << std::endl;
			// std::ostringstream filename;
			// filename << "/tmp/" << name << "_" << std::setw(3) << std::setfill('0') << file_idx++ << ".fc_log";
			// std::ofstream fout(filename.str().c_str());

			message = gelf::MessageBuilder(gelf::Severity::Error, "SAFTd Logdump");
			message.setHost(hostname);

			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			if (missing) {
				fullmsg << "# " << std::dec << missing << " missing entries\n";
			}
			if (full) {
				for (int i = idx; i < buffer.size(); ++i) {
					dumpline(fullmsg, i);
					++message_lines;
					gelf_post();
				}
			}
			for (int i = 0; i < idx; ++i) {
				dumpline(fullmsg, i);
				++message_lines;
				gelf_post();
			}

			struct timespec start = now;
			clock_gettime(CLOCK_REALTIME, &now);
			double dt = now.tv_sec-start.tv_sec;
			dt *= 1000;
			dt += (now.tv_nsec-start.tv_nsec)/1e6;
			fullmsg << "# logdump took " << dt << " ms\n" << std::endl;

			gelf_post(true);
			
			full    = false;
			missing = 0;
			idx     = 0;
	}
	void FCLogger::dump_file(std::ostream &out) {
		std::cerr << "dump_file" << std::endl;
		out << "\"timestamp\",\"source\",\"message\",\"full_message\"" << std::endl;

		time_t timer;
		char time_buffer[26];
		struct tm* tm_info;
		timer = time(NULL);
		tm_info = localtime(&timer);
		strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

		fullmsg.str("");
		if (missing) {
			fullmsg << "# " << std::dec << missing << " missing entries\n";
		}
		if (full) {
			for (int i = idx; i < buffer.size(); ++i) {
				dumpline(fullmsg, i);
			}
		}
		for (int i = 0; i < idx; ++i) {
			dumpline(fullmsg, i);
		}

		std::string fullmsgstr = fullmsg.str();
		if (fullmsgstr.size() && fullmsgstr.back()=='\n') fullmsgstr.pop_back();
		out << "\"" << time_buffer << "\",\"" << "hostname" << "\",\"SAFTd Logdump\",\"" << fullmsgstr << "\"" << std::endl;

		full    = false;
		missing = 0;
		idx     = 0;

	}

	void FCLogger::dump(bool force) {
		// if the force variable is true, checking and manipulating num_dumps is disabled
		if (log_level == 0) return;
		if (num_dumps == 0 && !force) return;

		if (logfilename.size() > 2) {
			std::ofstream out(logfilename.c_str(), std::ios::app);
			dump_file(out);
		} else {
			dump_gelf();
		}
		if (num_dumps > 0 && !force) --num_dumps;
	}

	void FCLogger::resize(unsigned new_size) {
		buffer.resize(new_size,LogEntry());
		full = false;
		missing = 0;
		idx = 0;
		file_idx = 0;
		message_lines = 0;
	}
	void FCLogger::set_level(unsigned new_level) {
		log_level = new_level;
	}
	void FCLogger::set_num_dumps(int new_num_dumps) {
		num_dumps = new_num_dumps;
	}

	void FCLogger::get_status(unsigned &size, unsigned &level, int &num, std::string &filename)
	{
		size = buffer.size();
		level = log_level;
		num = num_dumps;
		filename = logfilename;
	}


}

saftbus::FCLogger fc_logger("saftd",999,3,10);