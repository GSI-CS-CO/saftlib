#ifndef CLOG_H
#define CLOG_H

#include <syslog.h>
#include <ostream>
#include <string>

namespace saftlib {

enum LogPriority {
  kLogEmerg   = LOG_EMERG,   // system is unusable
  kLogAlert   = LOG_ALERT,   // action must be taken immediately
  kLogCrit    = LOG_CRIT,    // critical conditions
  kLogErr     = LOG_ERR,     // error conditions
  kLogWarning = LOG_WARNING, // warning conditions
  kLogNotice  = LOG_NOTICE,  // normal, but significant, condition
  kLogInfo    = LOG_INFO,    // informational message
  kLogDebug   = LOG_DEBUG    // debug-level message
};

class CLogBuffer : public std::streambuf
{
  public:
    explicit CLogBuffer(const char *ident, int facility);

  protected:
    int sync();
    int overflow(int c);

  private:
    std::string buffer;
    int priority;
  
  friend class CLogStream;
};

class CLogStream : public std::ostream
{
  public:
    CLogStream(const char *ident, int facility);
    CLogStream& operator << (LogPriority priority);
  private:
    CLogBuffer buffer;
};

// global syslog stream
extern CLogStream clog;

}

#endif
