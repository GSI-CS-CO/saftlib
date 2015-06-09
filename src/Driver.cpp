#define ETHERBONE_THROWS 1

#include "Driver.h"

namespace saftlib {

static DriverBase *top = 0;

void DriverBase::insert_self()
{
  next = top;
  top = this;
}

void DriverBase::remove_self()
{
  DriverBase **i;
  for (i = &top; *i != this; i = &(*i)->next) { }
  *i = next;
}

void Drivers::probe(OpenDevice& od)
{
  for (DriverBase *i = top; i; i = i->next)
    i->probe(od);
}

}
