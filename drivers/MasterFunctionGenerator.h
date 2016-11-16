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
#ifndef MASTER_FUNCTION_GENERATOR_H
#define MASTER_FUNCTION_GENERATOR_H

#include <deque>

#include "interfaces/MasterFunctionGenerator.h"
#include "FunctionGenerator.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;


class MasterFunctionGenerator : public Owned, public iMasterFunctionGenerator
{
  public:
    typedef MasterFunctionGenerator_Service ServiceType;
    struct ConstructorType {
      Glib::ustring objectPath;
      TimingReceiver* dev;
 			std::vector<Glib::RefPtr<FunctionGenerator>> functionGenerators;      
    };
    
    static Glib::RefPtr<MasterFunctionGenerator> create(const ConstructorType& args);
    
    // iMasterFunctionGenerator overrides
    void Arm();
    void Abort();
    bool AppendParameterSet(const std::vector< gint16 >& coeff_a, const std::vector< gint16 >& coeff_b, const std::vector< gint32 >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b, bool arm);
    void Flush();
    void setStartTag(guint32 val);
    guint32 getStartTag() const;
        
  protected:
    MasterFunctionGenerator(const ConstructorType& args);
    ~MasterFunctionGenerator();
    void Reset();

    void ownerQuit();
    
    TimingReceiver* dev;
  	std::vector<Glib::RefPtr<FunctionGenerator>> functionGenerators;      
  	
    guint32 startTag;
    
    struct ParameterTuple {
      gint16 coeff_a;
      gint16 coeff_b;
      gint32 coeff_c;
      guint8 step;
      guint8 freq;
      guint8 shift_a;
      guint8 shift_b;
      
      guint64 duration() const;
    };

};

}

#endif
