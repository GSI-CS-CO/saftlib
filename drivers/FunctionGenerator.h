#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include <deque>

#include "interfaces/FunctionGenerator.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class FunctionGenerator : public iFunctionGenerator, public Owned
{
  public:
    typedef FunctionGenerator_Service ServiceType;
    struct ConstructorType {
      TimingReceiver* dev;
      eb_address_t fgb;
      eb_address_t swi;
      unsigned num_channels;
      unsigned buffer_size;
      int index;
      guint32 macro;
    };
    
    static Glib::RefPtr<FunctionGenerator> create(const Glib::ustring& objectPath, ConstructorType args);
    
    // iFunctionGenerator overrides
    void AppendParameterSet(const std::vector< gint16 >& coeff_a, const std::vector< gint16 >& coeff_b, const std::vector< gint32 >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);
    void Flush();
    guint32 getVersion() const;
    unsigned char getSCUbusSlot() const;
    unsigned char getDeviceNumber() const;
    bool getEnabled() const;
    guint32 getStartTag() const;
    bool getRunning() const;
    guint64 getFillLevel() const;
    guint64 getSafeFillLevel() const;
    bool getAboveSafeFillLevel() const;
    unsigned char getOutputWindowSize() const;
    guint16 getExecutedParameterCount() const;
    void setEnabled(bool val);
    void setStartTag(guint32 val);
    void setSafeFillLevel(guint64 val);
    
    // sigc::signal< void, bool > Enabled;
    // sigc::signal< void, bool > AboveSafeFillLevel;
    // sigc::signal< void , guint64 > Started;
    // sigc::signal< void , guint64 , bool , bool > Stopped;
    
  protected:
    FunctionGenerator(ConstructorType args);
    ~FunctionGenerator();
    void updateAboveSafeFillLevel();
    void irq_handler(eb_data_t status);
    void refill();
    void releaseChannel();
    int acquireChannel();
    
    TimingReceiver* dev;
    eb_address_t shm;
    eb_address_t swi;
    unsigned num_channels;
    unsigned buffer_size;
    int index;
    unsigned char scubusSlot;
    unsigned char deviceNumber;
    unsigned char version;
    unsigned char outputWindowSize;
    eb_address_t irq;

    int channel; // -1 if no channel assigned
    bool enabled;
    bool running;
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
    
    // These 4 variables must be kept in sync:
    bool aboveSafeFillLevel;
    guint64 safeFillLevel;
    guint64 fillLevel;
    unsigned filled; // # of fifo entries currently on LM32
    std::deque<ParameterTuple> fifo;
};

}

#endif
