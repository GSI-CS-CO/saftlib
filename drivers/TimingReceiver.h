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
#ifndef TIMING_RECEIVER_H
#define TIMING_RECEIVER_H

#include "OpenDevice.h"
#include "ActionSink.h"
#include "EventSource.h"
#include "interfaces/TimingReceiver.h"
#include <memory>

namespace saftlib {

class TimingReceiver : public BaseObject, public iTimingReceiver, public iDevice {
  public:
    struct ConstructorType {
      Device device;
      std::string name;
      std::string etherbonePath;
      std::string objectPath;
      etherbone::sdb_msi_device base;
      eb_address_t stream;
      eb_address_t info;
      eb_address_t watchdog;
      eb_address_t pps;
      eb_address_t ats;
    };
    typedef TimingReceiver_Service ServiceType;
    
    static void probe(OpenDevice& od);
    TimingReceiver(const ConstructorType& args);
    ~TimingReceiver();
    
    // iDevice
    void Remove();
    std::string getEtherbonePath() const;
    std::string getName() const;
    
    // iTimingReceiver
    // saftbus method
    std::string NewSoftwareActionSink(const std::string& name);
    // saftbus method
    void InjectEvent(uint64_t event, uint64_t param, uint64_t time);
    // saftbus method
    void InjectEvent(uint64_t event, uint64_t param, saftlib::Time time);
    // saftbus method
    uint64_t ReadCurrentTime();
    // saftbus method
    saftlib::Time CurrentTime();
    std::map< std::string, std::string > getGatewareInfo() const;
    std::string getGatewareVersion() const;
    // saftbus property
    bool getLocked() const;
    // saftbus property
    void setLocked(bool l);
    bool getTemperatureSensorAvail() const;
    // saftbus method
    int32_t CurrentTemperature();
    // saftbus property
    std::map< std::string, std::string > getSoftwareActionSinks() const;
    // saftbus property
    std::map< std::string, std::string > getOutputs() const;
    // saftbus property
    std::map< std::string, std::string > getInputs() const;
    // saftbus property
    std::map< std::string, std::map< std::string, std::string > > getInterfaces() const;
    uint32_t getFree() const;

    // saftbus signal
    sigc::signal< void , uint64_t , uint64_t , uint64_t , uint64_t , uint16_t > MySignal;
    // saftbus signal
    sigc::signal< void , std::string , uint64_t , uint64_t , uint64_t , uint16_t > MySignal2;
    
    // Compile the condition table
    void compile();

    // Allow hardware access to the underlying device
    Device& getDevice() { return device; }
    eb_address_t getBase() { return base; }
    uint64_t ReadRawCurrentTime();
    
    // public type, even though the member is private
    typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)
    typedef std::map< SinkKey, std::shared_ptr<ActionSink> >  ActionSinks;
    typedef std::map< SinkKey, std::shared_ptr<EventSource> > EventSources;
    
  protected:
    mutable saftlib::Device device;
    std::string name;
    std::string etherbonePath;
    eb_address_t base;
    eb_address_t stream;
    eb_address_t watchdog;
    eb_address_t pps;
    eb_address_t ats;
    uint64_t sas_count;
    eb_address_t arrival_irq;
    eb_address_t generator_irq;
    
    std::map<std::string, std::string> info;
    mutable bool locked;
    mutable int32_t temperature;
    eb_data_t watchdog_value;
    
    sigc::connection pollConnection;
    unsigned channels;
    unsigned search_size;
    unsigned walker_size;
    unsigned max_conditions;
    unsigned used_conditions;
    std::vector<eb_address_t> channel_msis;
    std::vector<eb_address_t> queue_addresses;
    std::vector<uint16_t> most_full;
        
    typedef std::map< std::string, std::shared_ptr<Owned> > Owneds;
    typedef std::map< std::string, Owneds >              OtherStuff;
    
    ActionSinks  actionSinks;
    EventSources eventSources;
    OtherStuff   otherStuff;
    
    // Polling method
    bool poll();
    
    void setupGatewareInfo(uint32_t address);
    void do_remove(SinkKey key);
    void setHandler(unsigned channel, bool enable, 
                    eb_address_t address);
    void msiHandler(eb_data_t msi, unsigned channel);
    uint16_t updateMostFull(unsigned channel); // returns current fill
    void resetMostFull(unsigned channel);
    void popMissingQueue(unsigned channel, unsigned num);
  
  friend class ActionSink;
};
// hallo welt
}

#endif
