#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "FunctionGenerator.h"

namespace saftlib {

FunctionGenerator::FunctionGenerator(ConstructorType args)
{
}

const char *FunctionGenerator::getInterfaceName() const
{
  return "FunctionGenerator";
}

Glib::RefPtr<FunctionGenerator> FunctionGenerator::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<FunctionGenerator>::create(objectPath, args);
}

void FunctionGenerator::AppendParameterSet(const std::vector< gint16 >& coeff_a, const std::vector< gint16 >& coeff_b, const std::vector< gint32 >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b)
{
}

guint32 FunctionGenerator::getVersion() const
{
}

unsigned char FunctionGenerator::getSCUbusSlot() const
{
}

unsigned char FunctionGenerator::getDeviceNumber() const
{
}

bool FunctionGenerator::getEnabled() const
{
}

guint32 FunctionGenerator::getStartTag() const
{
}

bool FunctionGenerator::getRunning() const
{
}

guint64 FunctionGenerator::getFillLevel() const
{
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
}

void FunctionGenerator::setStartTag(guint32 val)
{
}

void FunctionGenerator::setSafeFillLevel(guint64 val)
{
}

}
