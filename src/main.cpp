#include <giomm.h>
#include <glibmm.h>
#include <iostream>

#include "ECA.h"

class ECA : public saftlib::ECA_Service {
  public:
    void Listen(
      const guint64& event,
      const unsigned char& bits,
      const guint64& offset,
      std::map< std::string, std::vector< gint32 > >& data,
      gint16& result) 
    {
      result = event;
      for (std::map< std::string, std::vector< gint32 > >::iterator i = data.begin();
           i != data.end(); ++i) {
        std::cout << i->first << " => [";
        for (unsigned j = 0; j < i->second.size(); ++j)
          std::cout << " " << i->second[j] << std::endl;
        std::cout << " ]" << std::endl;
      }
    }
    void Poke();
    ~ECA();
};


void ECA::Poke() {
  std::cout << "Got poked!\n" << std::endl;
}

ECA::~ECA() {
  std::cerr << "Destructor?!" << std::endl;
}

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    Glib::RefPtr<saftlib::ECA_Service> eca(new ECA);
    
    // registered_id = 
    eca->register_object(connection, "/de/gsi/saftlib/ECA");
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
  Glib::init();
  Gio::init();

  const guint id = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::ptr_fun(&on_name_acquired),
    sigc::ptr_fun(&on_name_lost));

  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  loop->run();

  // connection->unregister_object(registered_id);
  Gio::DBus::unown_name(id);

  return EXIT_SUCCESS;
}
