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
#include "FunctionGeneratorImpl.h"
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
 			//std::vector<Glib::RefPtr<FunctionGeneratorImpl>> functionGenerators;
 			std::vector<std::shared_ptr<FunctionGeneratorImpl>> functionGenerators;            
    };
    
    static Glib::RefPtr<MasterFunctionGenerator> create(const ConstructorType& args);
   
    // iMasterFunctionGenerator overrides
    void Arm();
    void Abort(bool);
		bool AppendParameterSets(const std::vector< std::vector< gint16 > >& coeff_a, const std::vector< std::vector< gint16 > >& coeff_b, const std::vector< std::vector< gint32 > >& coeff_c, const std::vector< std::vector< unsigned char > >& step, const std::vector< std::vector< unsigned char > >& freq, const std::vector< std::vector< unsigned char > >& shift_a, const std::vector< std::vector< unsigned char > >& shift_b, bool arm, bool wait_for_arm_ack);    
    std::vector<guint32> ReadExecutedParameterCounts();
    std::vector<guint64> ReadFillLevels();
    void Flush();
    void setStartTag(guint32 val);
    guint32 getStartTag() const;

    void setGenerateIndividualSignals(bool);
    bool getGenerateIndividualSignals() const;

    std::vector<Glib::ustring> ReadAllNames();
    std::vector<Glib::ustring> ReadNames();
    std::vector<int> ReadArmed();
    std::vector<int> ReadEnabled();
    void ResetActiveFunctionGenerators();
    void SetActiveFunctionGenerators(const std::vector<Glib::ustring>&);

  protected:
    MasterFunctionGenerator(const ConstructorType& args);
    ~MasterFunctionGenerator();
    
    void arm_all();
    void reset_all();

    void ownerQuit();
    
    void on_fg_running(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_armed(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_enabled(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_started(std::shared_ptr<FunctionGeneratorImpl>& fg, guint64);
    void on_fg_stopped(std::shared_ptr<FunctionGeneratorImpl>& fg, guint64 time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow);
    void on_fg_refill(std::shared_ptr<FunctionGeneratorImpl>& fg);


    TimingReceiver* dev;
  	std::vector<std::shared_ptr<FunctionGeneratorImpl>> allFunctionGenerators;      
  	std::vector<std::shared_ptr<FunctionGeneratorImpl>> activeFunctionGenerators;      
    guint32 startTag;
    bool generateIndividualSignals;
    
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
