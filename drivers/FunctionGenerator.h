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
#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include <deque>

#include "interfaces/FunctionGenerator.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class FunctionGeneratorChannelAllocation : public Glib::Object
{
  public:
    std::vector<int> indexes;
};

class FunctionGenerator : public Owned, public iFunctionGenerator
{
  public:
    typedef FunctionGenerator_Service ServiceType;
    struct ConstructorType {
      Glib::ustring objectPath;
      TimingReceiver* dev;
      Glib::RefPtr<FunctionGeneratorChannelAllocation> allocation;
      eb_address_t fgb;
      eb_address_t swi;
      etherbone::sdb_msi_device base;
      sdb_device mbx;
      unsigned num_channels;
      unsigned buffer_size;
      unsigned int index;
      guint32 macro;
    };
    
    static Glib::RefPtr<FunctionGenerator> create(const ConstructorType& args);
    
    // iFunctionGenerator overrides
    void Arm();
    void Abort();
    guint64 ReadFillLevel();
    bool AppendParameterSet(const std::vector< gint16 >& coeff_a, const std::vector< gint16 >& coeff_b, const std::vector< gint32 >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);
    void Flush();
    guint32 getVersion() const;
    unsigned char getSCUbusSlot() const;
    unsigned char getDeviceNumber() const;
    unsigned char getOutputWindowSize() const;
    bool getEnabled() const;
    bool getArmed() const;
    bool getRunning() const;
    guint32 getStartTag() const;
    guint32 ReadExecutedParameterCount();
    void setStartTag(guint32 val);
    
  protected:
    FunctionGenerator(const ConstructorType& args);
    ~FunctionGenerator();
    bool lowFill() const;
    void irq_handler(eb_data_t msi);
    void refill();
    void releaseChannel();
    void acquireChannel();
    void Reset();
    bool ResetFailed();
    void ownerQuit();
    
    TimingReceiver* dev;
    Glib::RefPtr<FunctionGeneratorChannelAllocation> allocation;
    eb_address_t shm;
    eb_address_t swi;
    etherbone::sdb_msi_device base;
    sdb_device mbx;
    unsigned num_channels;
    unsigned buffer_size;
    unsigned int index;
    unsigned char scubusSlot;
    unsigned char deviceNumber;
    unsigned char version;
    unsigned char outputWindowSize;
    eb_address_t irq;

    int channel; // -1 if no channel assigned
    bool enabled;
    bool armed;
    bool running;
    bool abort;
    sigc::connection resetTimeout;
    guint32 startTag;
    unsigned executedParameterCount;
    
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

    unsigned mbx_slot;
    
    // These 3 variables must be kept in sync:
    guint64 fillLevel;
    unsigned filled; // # of fifo entries currently on LM32
    std::deque<ParameterTuple> fifo;
};

}

#endif
