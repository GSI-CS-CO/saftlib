#ifndef SAFTLIB_DIRECTORY_H
#define SAFTLIB_DIRECTORY_H

#include "interfaces/Directory.h"
#include "Device.h"

namespace saftlib {

class Directory : public Directory_Service
{
  public:
    static const Glib::RefPtr<Directory>& get();
    
    const Glib::RefPtr<Glib::MainLoop>&        loop()       { return m_loop; }
    const Glib::RefPtr<Gio::DBus::Connection>& connection() { return m_connection; }
    
    void setConnection(const Glib::RefPtr<Gio::DBus::Connection>& connection);
    void resetConnection();
    
    void AttachDevice(const Glib::ustring& name, const Glib::ustring& path);
    void Quit();
    std::map< Glib::ustring, Glib::ustring > getDevices() const { return devs; }
    
  protected:
    Directory();
    
    Glib::RefPtr<Glib::MainLoop> m_loop;
    Glib::RefPtr<Gio::DBus::Connection> m_connection;
    
    std::map< Glib::ustring, Glib::ustring > devs;
};

} // saftlib

#endif
