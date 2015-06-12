#ifndef SAFTLIB_DIRECTORY_H
#define SAFTLIB_DIRECTORY_H

#include "interfaces/Directory.h"
#include "OpenDevice.h"

namespace saftlib {

class Directory : public iDirectory, public Glib::Object
{
  public:
    static const Glib::RefPtr<Directory>& get();
    ~Directory();
    
    void setConnection(const Glib::RefPtr<Gio::DBus::Connection>& connection);
    
    const Glib::RefPtr<Glib::MainLoop>&        loop()       { return m_loop; }
    const Glib::RefPtr<Gio::DBus::Connection>& connection() { return m_connection; }
    
    Glib::ustring AttachDevice(const Glib::ustring& name, const Glib::ustring& path);
    void RemoveDevice(const Glib::ustring& name);
    void Quit();
    std::map< Glib::ustring, Glib::ustring > getDevices() const;
    
  protected:
    Directory();
    
    Directory_Service m_service;
    Glib::RefPtr<Glib::MainLoop> m_loop;
    Glib::RefPtr<Gio::DBus::Connection> m_connection;
    etherbone::Socket socket;
    sigc::connection eb_source;
    
    std::map< Glib::ustring, OpenDevice > devs;
};

} // saftlib

#endif
