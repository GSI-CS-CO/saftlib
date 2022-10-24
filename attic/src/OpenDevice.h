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
#ifndef SAFTLIB_OPEN_DEVICE_H
#define SAFTLIB_OPEN_DEVICE_H

#include "Device.h"
#include "BaseObject.h"

namespace saftlib {

struct OpenDevice {
  Device device;
  std::string name;
  std::string objectPath;
  std::string etherbonePath;
  std::shared_ptr<BaseObject> ref;
  
  OpenDevice(etherbone::Device d, eb_address_t first, eb_address_t last, bool poll = false);
};

}

#endif
