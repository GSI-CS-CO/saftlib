#define ETHERBONE_THROWS 1

#include <iostream>

#include "RegisteredObject.h"
#include "FunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"

namespace saftlib {

FunctionGenerator::FunctionGenerator(ConstructorType args)
 : dev(args.dev), fgb(args.fgb), swi(args.swi), channel(args.channel)
{
  etherbone::Cycle cycle;
  eb_data_t d_scubusSlot, d_deviceNumber, d_version;

  // different device, cannot be in same cycle
  dev->getDevice().write(swi + SWI_STATUS, EB_DATA32, channel);
  Glib::usleep(10000); // ... lol

  cycle.open(dev->getDevice());
  cycle.read(fgb + FGSTAT + 0x0, EB_DATA32, &d_scubusSlot);
  cycle.read(fgb + FGSTAT + 0x4, EB_DATA32, &d_deviceNumber);
  cycle.read(fgb + FGSTAT + 0x8, EB_DATA32, &d_version);
  cycle.close();
  std::cout << "Info: " << d_scubusSlot << " " << d_deviceNumber << " " << d_version << std::endl;

  safeFillLevel = 100000;
  startTag = 0xfeedbabe; // !!! missing hardware

  version = d_version;
  scubusSlot = d_scubusSlot;
  deviceNumber = d_deviceNumber;
}

Glib::RefPtr<FunctionGenerator> FunctionGenerator::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<FunctionGenerator>::create(objectPath, args);
}

void FunctionGenerator::AppendParameterSet(
  const std::vector< gint16 >& coeff_a,
  const std::vector< gint16 >& coeff_b,
  const std::vector< gint32 >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b)
{
  ownerOnly();
}

void FunctionGenerator::Flush()
{
  ownerOnly();
}

guint32 FunctionGenerator::getVersion() const
{
  return version;
}

unsigned char FunctionGenerator::getSCUbusSlot() const
{
  return scubusSlot;
}

unsigned char FunctionGenerator::getDeviceNumber() const
{
  return deviceNumber;
}

bool FunctionGenerator::getEnabled() const
{
  return enabled;
}

guint32 FunctionGenerator::getStartTag() const
{
  return startTag;
}

bool FunctionGenerator::getRunning() const
{
}

guint64 FunctionGenerator::getFillLevel() const
{
  // !!! check update of AboveSafeFillLevel
}

guint64 FunctionGenerator::getSafeFillLevel() const
{
}

bool FunctionGenerator::getAboveSafeFillLevel() const
{
}

unsigned char FunctionGenerator::getOutputWindowSize() const
{
}

guint16 FunctionGenerator::getExecutedParameterCount() const
{
}

void FunctionGenerator::setEnabled(bool val)
{
  ownerOnly();
}

void FunctionGenerator::setStartTag(guint32 val)
{
  ownerOnly();
  if (val != startTag) {
    // !!! missing hardware
    startTag = val;
    StartTag(startTag);
  }
}

void FunctionGenerator::setSafeFillLevel(guint64 val)
{
  ownerOnly();
  if (val != safeFillLevel) {
    safeFillLevel = val;
    SafeFillLevel(safeFillLevel);
  }
  // check not above safe level
  getFillLevel();
}

}
