#ifndef SAFTLIB_SAFTD_H
#define SAFTLIB_SAFTD_H

#include "interfaces/SAFTd.h"
#include "OpenDevice.h"

namespace saftlib {

class SAFTd : public iSAFTd
{
  public:
    static SAFTd& get() { return saftd; }
    ~SAFTd();

    void setConnection(const Glib::RefPtr<Gio::DBus::Connection>& connection);

    const Glib::RefPtr<Glib::MainLoop>&        loop()       { return m_loop; }
    const Glib::RefPtr<Gio::DBus::Connection>& connection() { return m_connection; }

    void AttachNonEtherboneDevice(const Glib::ustring& name, Glib::RefPtr<Glib::Object> device);
    Glib::ustring AttachDevice(const Glib::ustring& name, const Glib::ustring& path);
    void RemoveDevice(const Glib::ustring& name);
    void Quit();
    std::map< Glib::ustring, Glib::ustring > getDevices() const;

    Glib::ustring getSourceVersion() const;
    Glib::ustring getBuildInfo() const;

  protected:
    SAFTd();

    SAFTd_Service m_service;
    Glib::RefPtr<Glib::MainLoop> m_loop;
    Glib::RefPtr<Gio::DBus::Connection> m_connection;
    etherbone::Socket socket;
    sigc::connection eb_source;
    sigc::connection msi_source;

    std::map< Glib::ustring, OpenDevice > devs;

    std::map< Glib::ustring, Glib::RefPtr<Glib::Object> > otherDevs;

    static SAFTd saftd;
};

} // saftlib

#endif
