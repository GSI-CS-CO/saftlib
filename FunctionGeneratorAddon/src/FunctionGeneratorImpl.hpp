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
#ifndef FUNCTION_GENERATOR_IMPL_HPP_
#define FUNCTION_GENERATOR_IMPL_HPP_

#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#include <etherbone.h>


#include <deque>
#include <memory>
#include <boost/circular_buffer.hpp>
#include <sigc++/sigc++.h>
#include <saftlib/Time.hpp>
#include <saftlib/Mailbox.hpp>

#include <saftbus/loop.hpp>


namespace saftlib {

class SAFTd;
class TimingReceiver;
// class Mailbox;

// class FunctionGeneratorChannelAllocation //: public Glib::Object
// {
//   public:
//     std::vector<int> indexes;
// };

  struct ParameterTuple {
      int16_t coeff_a;
      int16_t coeff_b;
      int32_t coeff_c;
      uint8_t step;
      uint8_t freq;
      uint8_t shift_a;
      uint8_t shift_b;
    
      uint64_t duration() const;  
    };

class FunctionGeneratorImpl //: public Glib::Object
{
	friend class MasterFunctionGenerator;
	
  public:
//    typedef FunctionGenerator_Service ServiceType;
    // struct ConstructorType {
    //   std::string objectPath;
    //   TimingReceiver *tr;
    //   std::shared_ptr<std::vector<int> > allocation;
    //   eb_address_t fgb;
    //   eb_address_t swi;
    //   etherbone::sdb_msi_device base;
    //   sdb_device mbx;
    //   unsigned num_channels;
    //   unsigned buffer_size;
    //   unsigned int index;
    //   uint32_t macro;
    // };

    FunctionGeneratorImpl(SAFTd *saft_daemon
                        , TimingReceiver *timing_receiver
                        , std::shared_ptr<std::vector<int> > channel_allocation
                        , eb_address_t fgb
                        , int mbox_slot
                        , unsigned n_channels
                        , unsigned buf_size
                        , unsigned idx
                        , uint32_t macro);
    // FunctionGeneratorImpl(const ConstructorType& args);
    ~FunctionGeneratorImpl();
    
    //static std::shared_ptr<FunctionGenerator> create(const ConstructorType& args);
    
    template<typename Iter> bool appendParameterTuples(Iter it, Iter end)
    {
      for (; it != end; ++it)
      {
        fifo_push_back(*it);
        fillLevel += (*it).duration();
      }

      if (channel != -1) refill(false);
      return lowFill();
    }

    bool appendParameterTuples(std::vector<ParameterTuple> parameters);


    void Arm();
    void Abort();
    uint64_t ReadFillLevel();
    bool appendParameterSet(const std::vector< int16_t >& coeff_a, const std::vector< int16_t >& coeff_b, const std::vector< int32_t >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);
    void Flush();
    uint32_t getVersion() const;
    unsigned char getSCUbusSlot() const;
    unsigned char getDeviceNumber() const;
    unsigned char getOutputWindowSize() const;
    bool getEnabled() const;
    bool getArmed() const;
    bool getRunning() const;
    uint32_t getStartTag() const;
    uint32_t ReadExecutedParameterCount();
    void setStartTag(uint32_t val);

    std::string GetName();

    sigc::signal<void, bool> signal_enabled;
    sigc::signal<void, bool> signal_running;
    sigc::signal<void, bool> signal_armed;
    sigc::signal<void> signal_refill;
    sigc::signal<void, uint64_t> signal_started;
    sigc::signal<void, uint64_t, bool, bool, bool> signal_stopped;

    void flush();
    void arm();
    void Reset();
    
    
    
  protected:
    bool lowFill() const;
    void irq_handler(eb_data_t msi);
    void refill(bool);
    void releaseChannel();
    void acquireChannel();

    bool ResetFailed();
    void ownerQuit();

    void fifo_push_back(const ParameterTuple& tuple);

            
            
    SAFTd          *saftd;
    TimingReceiver *tr;
    Mailbox        *mbx;
    etherbone::Device &device;
    std::shared_ptr<std::vector<int> > allocation;
    eb_address_t shm;
    // eb_address_t swi;
    // etherbone::sdb_msi_device base;
    // sdb_device mbx;
    int mb_slot; // host->lm32 mailbox slot (owned by lm32, therefore we have only the index)
    unsigned num_channels;
    unsigned buffer_size;
    unsigned int index;
    unsigned char scubusSlot;
    unsigned char deviceNumber;
    unsigned char version;
    unsigned char outputWindowSize;
    eb_address_t irq;
    std::unique_ptr<Mailbox::Slot> host_slot; // lm32->host (owned by host, therefore we have a Mailbox::Slot object)

    int channel; // -1 if no channel assigned
    bool enabled;
    bool armed;
    bool running;
    bool abort;

    //sigc::connection resetTimeout;
    saftbus::SourceHandle resetTimeout;
    uint32_t startTag;
    unsigned executedParameterCount;

    unsigned mbx_slot;
    eb_address_t mailbox_slot_address;
    
    // These 3 variables must be kept in sync:
    uint64_t fillLevel;
    unsigned filled; // # of fifo entries currently on LM32
    boost::circular_buffer<ParameterTuple> fifo;

    unsigned fg_fifo_max_size;
};

}

#endif
