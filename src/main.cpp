#include <iostream>
#include <giomm.h>

#include "ECA.h"
#include "eb-source.h"

class ECA : public saftlib::ECA_Service {
  public:
    ECA();
    void Poke();
    void Listen(
      const guint64& event,
      const unsigned char& bits,
      const guint64& offset,
      const std::map< Glib::ustring, std::vector< gint32 > >& data, 
      gint16& result);
};

ECA::ECA()
{
 setFrequency("125MHz");
}

void ECA::Poke() 
{
  Cry("That hurt!");
  std::cout << "Got poked!" << std::endl;
  setName("xx");
}

void ECA::Listen(
  const guint64& event,
  const unsigned char& bits,
  const guint64& offset,
  const std::map< Glib::ustring, std::vector< gint32 > >& data,
  gint16& result)
{
  result = event;
  for (std::map< Glib::ustring, std::vector< gint32 > >::const_iterator i = data.begin();
       i != data.end(); ++i) {
    std::cout << i->first << " => [";
    for (unsigned j = 0; j < i->second.size(); ++j)
      std::cout << " " << i->second[j];
    std::cout << " ]" << std::endl;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    Glib::RefPtr<saftlib::ECA_Service> eca(new ECA);
    
    eca->register_self(connection, "/de/gsi/saftlib/ECA");
  } catch(const Glib::Error& ex) {
    std::cerr << "Registration of object failed." << std::endl;
  }
}

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, const Glib::ustring& /* name */)
{
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
}

int main(int, char**)
{
  std::locale::global(std::locale(""));
  Gio::init();
  
  etherbone::Socket socket;
  socket.open();

  const guint id = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::ptr_fun(&on_name_acquired),
    sigc::ptr_fun(&on_name_lost));
  
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  eb_attach_source(loop, socket);
  loop->run();

  Gio::DBus::unown_name(id);

  return EXIT_SUCCESS;
}
