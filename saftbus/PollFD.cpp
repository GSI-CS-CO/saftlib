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
