#ifndef EB_SOURCE_H
#define EB_SOURCE_H

#include <glibmm.h>
#include <etherbone.h>

sigc::connection eb_attach_source(const Glib::RefPtr<Glib::MainLoop>& loop, etherbone::Socket socket);

#endif
