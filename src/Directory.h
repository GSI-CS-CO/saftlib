#ifndef SAFTLIB_DIRECTORY_H
#define SAFTLIB_DIRECTORY_H

#include "interfaces/Directory.h"
#include "Device.h"

namespace saftlib {

class Directory : public Directory_Service
{
  public:
    ~Directory();
    static const Glib::RefPtr<Directory>& get();
    
    void Quit();
    
    const Glib::RefPtr<Glib::MainLoop>&        loop()       { return m_loop; }
    const Glib::RefPtr<Gio::DBus::Connection>& connection() { return m_connection; }
    saftlib::Devices& devices() { return m_devices; }
    
    void setConnection(const Glib::RefPtr<Gio::DBus::Connection>& connection);
    void resetConnection();
    
    // only call add from within your drivers probe method!
    void add(const Glib::ustring& iface, const Glib::ustring& path, const Glib::RefPtr<Glib::Object>& object);
  
  protected:
    Directory();
    Glib::RefPtr<Glib::MainLoop> m_loop;
    saftlib::Devices m_devices;
    Glib::RefPtr<Gio::DBus::Connection> m_connection;
    std::vector< Glib::RefPtr<Glib::Object> > refs;
    std::map< Glib::ustring, std::vector< Glib::ustring> > devs;
};

} // saftlib

#endif
