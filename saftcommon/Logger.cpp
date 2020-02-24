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


}
