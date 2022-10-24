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
#include "PollFD.h"


namespace Slib {



PollFD::PollFD ()
{
	pfd.fd = 0;
	pfd.events = 0;
}

PollFD::PollFD (int fd)
{
	pfd.fd = fd;
	pfd.events = 0;
}

PollFD::PollFD (int fd, IOCondition events)
{
	pfd.fd = fd;
	pfd.events = events;
}

void PollFD::set_fd(int fd)
{
	pfd.fd = fd;
}

int PollFD::get_fd() const
{
	return pfd.fd;
}

void PollFD::set_events(IOCondition events)
{
	pfd.events = events;
}

IOCondition PollFD::get_events() const
{
	return static_cast<IOCondition>(pfd.events);
}

void PollFD::set_revents(IOCondition revents)
{
	pfd.revents = revents;
}

IOCondition PollFD::get_revents() const
{
	return static_cast<IOCondition>(pfd.revents);
}


bool PollFD::operator==(const PollFD &rhs)
{
	return pfd.fd == rhs.pfd.fd;
}

} // namespace Slib
