#include "ObjectRegistry.h"
#include "Driver.h"
#include "ECA.h"

namespace saftlib {

class ECA : public RegisteredObject<ECA_Service>
{
  public:
    ECA();
    void Poke();
};

ECA::ECA()
 : RegisteredObject<ECA_Service>("/de/gsi/saftlib/ECA")
{
  setFrequency("125MHz");
}

void ECA::Poke()
{
  Cry("That hurt!");
  setName("xx");
}

static Driver<ECA> eca;

} // saftlib
