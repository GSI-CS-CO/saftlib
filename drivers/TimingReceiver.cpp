#define ETHERBONE_THROWS 1

#include <iostream>
#include "RegisteredObject.h"
#include "Driver.h"
#include "interfaces/TimingReceiver.h"

namespace saftlib {

class TimingReceiver : public RegisteredObject<TimingReceiver_Service> {
  public:
    static void probe(OpenDevice& od);
    TimingReceiver(OpenDevice& device);
    
    // iDevice
    void Remove();
    Glib::ustring getEtherbonePath() const;
    Glib::ustring getName() const;
    
    // iTimingReceiver
    Glib::ustring NewSoftwareActionSink(const Glib::ustring& name);
    void InjectEvent(guint64 event, guint64 param, guint64 time);
    guint64 getCurrentTime() const;
    std::map< Glib::ustring, Glib::ustring > getSoftwareActionSinks() const;
    std::map< Glib::ustring, Glib::ustring > getOutputs() const;
    std::map< Glib::ustring, Glib::ustring > getInputs() const;
    std::map< Glib::ustring, Glib::ustring > getInoutputs() const;
    std::vector< Glib::ustring > getGuards() const;
    std::map< Glib::ustring, std::vector< Glib::ustring > > getInterfaces() const;
    guint32 getFree() const;

  protected:
    saftlib::Device dev;
};

TimingReceiver::TimingReceiver(OpenDevice& od)
 : RegisteredObject(od.path), dev(od.device) { 
}

void TimingReceiver::Remove()
{
  // !!!
}

Glib::ustring TimingReceiver::getEtherbonePath() const
{
  return "wtf";
}

Glib::ustring TimingReceiver::getName() const
{
  return "bah";
}

Glib::ustring TimingReceiver::NewSoftwareActionSink(const Glib::ustring& name)
{
  return ""; // !!!
}

void TimingReceiver::InjectEvent(guint64 event, guint64 param, guint64 time)
{
  // !!!
}

guint64 TimingReceiver::getCurrentTime() const
{
  return 1; // !!!
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getSoftwareActionSinks() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!!
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getOutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!!
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!!
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInoutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!!
}

std::vector< Glib::ustring > TimingReceiver::getGuards() const
{
  std::vector< Glib::ustring > out;
  return out; // !!!
}

std::map< Glib::ustring, std::vector< Glib::ustring > > TimingReceiver::getInterfaces() const
{
  std::map< Glib::ustring, std::vector< Glib::ustring > > out;
  return out; // !!!
}

guint32 TimingReceiver::getFree() const
{
  return 0; // !!!
}

void TimingReceiver::probe(OpenDevice& od)
{
  // !!! check board ID
  od.ref = Glib::RefPtr<Glib::Object>(new TimingReceiver(od));
}

static Driver<TimingReceiver> timingReceiver;

}
