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
#ifndef SAFTLIB_SAFTD_H
#define SAFTLIB_SAFTD_H

#include "interfaces/SAFTd.h"
#include "OpenDevice.h"

#include <saftbus.h>

namespace saftlib {

class SAFTd : public iSAFTd
{
  public:
    static SAFTd& get() { return saftd; }
    ~SAFTd();
    
    void setConnection(const Glib::RefPtr<saftbus::Connection>& connection);
    
    const Glib::RefPtr<Glib::MainLoop>&        loop()       { return m_loop; }
    const Glib::RefPtr<saftbus::Connection>& connection() { return m_connection; }
    
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
    Glib::RefPtr<saftbus::Connection> m_connection;
    etherbone::Socket socket;
    sigc::connection eb_source;
    sigc::connection msi_source;
    
    std::map< Glib::ustring, OpenDevice > devs;

    static SAFTd saftd;
};

} // saftlib

#endif
