#include "Logger.h"


namespace saftbus 
{

	Logger::Logger(const std::string &filename, bool flush_often) 
		: flush_after_log(flush_often)
		, file(filename.c_str())
	{
		//newMsg(0).add("logger up and running").log();
	}

	Logger& Logger::newMsg(int severity) {
		// msg.str("");
		// msg << std::setw(2)  << severity << "  ";
		// msg << std::setw(25) << getTimeTag() << ": ";
		return *this;
	}
	// Logger& Logger::add(const Glib::ustring &content) {
	// 	return add(content.c_str());
	// }	

	void Logger::log() {
		// file << msg.str() << "\n";
		// if (flush_after_log) {
		// 	file.flush();
		// }
	}

	std::string Logger::getTimeTag()
	{
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char buffer[80];
		strftime(buffer,80,"%c",timeinfo);
		std::string timestring(buffer);
		for (char ch: timestring) if (ch == ':') ch = '.';
		return timestring;
	}


}
