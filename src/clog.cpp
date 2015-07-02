#include "clog.h"

namespace saftlib {

CLogBuffer::CLogBuffer(const char *ident, int facility)
 : buffer(), priority(LOG_DEBUG)
{
  openlog(ident, LOG_PID|LOG_CONS, facility);
}

int CLogBuffer::sync()
{
  if (buffer.size()) {
    syslog(priority, "%s", buffer.c_str());
    buffer.clear();
    priority = LOG_DEBUG;
  }
  return 0;
}

int CLogBuffer::overflow(int c)
{
  if (c == traits_type::eof()) sync();
  else buffer += static_cast<char>(c);
  return c;
}

CLogStream::CLogStream(const char *ident, int facility)
 : std::ostream(&buffer), buffer(ident, facility)
{
}

CLogStream& CLogStream::operator << (LogPriority p)
{
  buffer.priority = p;
  return *this;
}

}
