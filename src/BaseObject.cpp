#define ETHERBONE_THROWS 1

#include "BaseObject.h"

namespace saftlib
{

BaseObject::BaseObject(const Glib::ustring& objectPath_)
 : objectPath(objectPath_)
{
}

}
