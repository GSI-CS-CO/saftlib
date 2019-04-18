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
#ifndef OBJECT_H
#define OBJECT_H

//#include <giomm.h>
#include <memory>
#include <saftbus.h>

namespace saftlib {

class BaseObject /*: public Glib::Object*/
{
  public:
    BaseObject(const std::string& objectPath);
    virtual ~BaseObject(); // need a virtual destructor
    
    // Most classes need this to build paths recursively
    const std::string& getObjectPath() const { return objectPath; }
    
    // provided by RegisteredObject
    virtual const std::string& getSender() const = 0;
    virtual const std::shared_ptr<saftbus::Connection>& getConnection() const = 0;
    
  protected:
    std::string objectPath;
};

}

#endif
