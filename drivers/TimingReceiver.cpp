#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "Driver.h"
#include "interfaces/TimingReceiver.h"

namespace saftlib {

class TimingReceiver : public iTimingReceiver, public iDevice, public Glib::Object {
  public:
    typedef OpenDevice& ConstructorType;
    typedef TimingReceiver_Service ServiceType;
    
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
    Glib::ustring name;
    Glib::ustring etherbonePath;
};

TimingReceiver::TimingReceiver(OpenDevice& od)
 : dev(od.device),
   name(od.name),
   etherbonePath(od.etherbonePath)
{
}

void TimingReceiver::Remove()
{
  Directory::get()->RemoveDevice(name);
}

Glib::ustring TimingReceiver::getEtherbonePath() const
{
  return etherbonePath;
}

Glib::ustring TimingReceiver::getName() const
{
  return name;
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
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInoutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::vector< Glib::ustring > TimingReceiver::getGuards() const
{
  std::vector< Glib::ustring > out;
  return out; // !!! not for cryring
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
  od.ref = RegisteredObject<TimingReceiver>::create(od.objectPath, od);
}

static Driver<TimingReceiver> timingReceiver;

}
