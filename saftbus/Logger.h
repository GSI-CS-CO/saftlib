#ifndef SAFTBUS_LOGGER_H_
#define SAFTBUS_LOGGER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <glibmm.h>

namespace saftbus 
{

class Logger 
{
public: 
	Logger(const std::string &filename, bool flush_often = true);

	Logger& newMsg(int severity);
	template<class T> 
	Logger& add(const T &content) {
		//std::cerr << content;
		//msg << content;
		return *this;
	}
	Logger& add(const Glib::ustring &content);
	void log();

private:

	std::string getTimeTag();

	bool flush_after_log;

	std::ostringstream msg;
	std::ofstream file;


};


} // namespace saftbus

#endif 
