#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include "interfaces/FunctionGenerator.h"
#include "Owned.h"

namespace saftlib {

class FunctionGenerator : public iFunctionGenerator, public Owned
{
  public:
    typedef FunctionGenerator_Service ServiceType;
    struct ConstructorType {
      eb_address_t fg;
      int channel;
    };
    
    static Glib::RefPtr<FunctionGenerator> create(Glib::ustring& objectPath, ConstructorType args);
    
    const char *getInterfaceName() const;
    
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
    // sigc::signal< void, guint32 > StartTag;
    // sigc::signal< void, guint64 > SafeFillLevel;
    // sigc::signal< void, bool > AboveSafeFillLevel;
    // sigc::signal< void , guint64 > Started;
    // sigc::signal< void , guint64 , bool , bool > Stopped;
    
  protected:
    FunctionGenerator(ConstructorType args);
    
    bool foo;
};

}

#endif
