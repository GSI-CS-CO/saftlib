/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
