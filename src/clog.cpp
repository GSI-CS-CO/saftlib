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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

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
