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

#include <loop.h>
#include <memory>

namespace saftlib {

class SAFTd : public iSAFTd
{
  public:
    static SAFTd& get() { return saftd; }
    ~SAFTd();
    
    void setConnection(const std::shared_ptr<saftbus::Connection>& connection);
    
    const std::shared_ptr<Slib::MainLoop>&      loop()       { return m_loop; }
    const std::shared_ptr<saftbus::Connection>& connection() { return m_connection; }
    
    // @saftbus-export
    std::string AttachDevice(const std::string& name, const std::string& path);
    // @saftbus-export
    void RemoveDevice(const std::string& name);
    // @saftbus-export
    std::string EbForward(const std::string& saftlib_device);
    // @saftbus-export
    void Quit();
    // @saftbus-export
    std::map< std::string, std::string > getDevices() const;
    
    // @saftbus-export
    std::string getSourceVersion() const;
    // @saftbus-export
    std::string getBuildInfo() const;
    
  protected:
    SAFTd();
    
    SAFTd_Service m_service;
    std::shared_ptr<Slib::MainLoop> m_loop;
    std::shared_ptr<saftbus::Connection> m_connection;
    std::map< std::string, std::shared_ptr<EB_Forward> > m_eb_forward; 
    etherbone::Socket socket;
    sigc::connection eb_source;
    sigc::connection msi_source;
    
    std::map< std::string, OpenDevice > devs;

    static SAFTd saftd;
};

} // saftlib

#endif