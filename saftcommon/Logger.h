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
#include <time.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>

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


};
class FCLogger
{
public:
	struct LogEntry {
		LogEntry() {
			t.tv_sec  = 0;
			t.tv_nsec = 0;
			file      = nullptr;
			line      = 0;
			func      = nullptr;
			what      = nullptr;
			who       = 0;
			memset(&text, 0, 32);
			param     = 0;
		}

		struct timespec t;
		const char * file;
		int          line;
		const char * func;
		const char * what;
		int          who; // PROXY=0, SAFTD=1, DRIVER=2
		char         text[32];
		int64_t          param;
		int64_t          dict;

	};

	std::string name;
	std::vector<LogEntry> buffer;
	bool full;
	long missing; // count the dropped log entries
	unsigned idx;

	FCLogger(std::string n, int size);
	void log(const char *file, int line, const char* func, const char* what, int who, const char *text, int64_t dict, int64_t param);
	void log_ts(struct timespec ts, const char *file, int line, const char* func, const char* what, int who, const char *text, int64_t dict, int64_t param);
	void dumpline(std::ostream &out, int idx);
	void dump();
	void dump(std::ostream &out);

	std::map<int, std::string> dict;

};

#define WHO_LOG_PROXY       0
#define WHO_LOG_SAFTD       1
#define WHO_LOG_DRIVER      2
#define WHO_LOG_MAINCONTEXT 3


}

extern saftbus::FCLogger fc_logger;
#define PROXY_LOGT(ts,w,txt,d,p) fc_logger.log_ts(ts,__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_PROXY,txt,d,p)
#define SAFTD_LOGT(w,txt,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_SAFTD,txt,d,p)
#define DRIVER_LOGT(w,txt,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_DRIVER,txt,d,p)
#define MAINCONTEXT_LOGT(w,txt,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_MAINCONTEXT,txt,d,p)

#define PROXY_LOG(ts,w,d,p) fc_logger.log_ts(ts,__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_PROXY,"",d,p)
#define SAFTD_LOG(w,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_SAFTD,"",d,p)
#define DRIVER_LOG(w,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_DRIVER,"",d,p)
#define MAINCONTEXT_LOG(w,d,p) fc_logger.log(__FILE__, __LINE__, __FUNCTION__,w,WHO_LOG_MAINCONTEXT,"",d,p)

#endif 
