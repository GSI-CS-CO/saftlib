#define ETHERBONE_THROWS 1

#include <cassert>
#include "RegisteredObject.h"
#include "SoftwareActionSink.h"
#include "SoftwareCondition.h"
#include "src/clog.h"

namespace saftlib {

SoftwareActionSink::SoftwareActionSink(ConstructorType args)
 : ActionSink(args.dev, args.channel, args.destroy),
   conflictCount(0), lateCount(0), delayedCount(0), actionCount(0),
   executeLate(false), generateDelayed(false),
   lastID(0), lastParam(0), lastTime(0)
{
}

const char *SoftwareActionSink::getInterfaceName() const
{
  return "SoftwareActionSink";
}

Glib::ustring SoftwareActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards)
{
  return NewConditionHelper(active, id, mask, offset, guards, 0,
    sigc::ptr_fun(&SoftwareCondition::create));
}

Glib::RefPtr<SoftwareActionSink> SoftwareActionSink::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareActionSink>::create(objectPath, args);
}

guint32 SoftwareActionSink::ReadConflictCount()
{
  return conflictCount;
}

guint32 SoftwareActionSink::ReadLateCount()
{
  return lateCount;
}

guint32 SoftwareActionSink::ReadDelayedCount()
{
  return delayedCount;
}

guint32 SoftwareActionSink::ReadActionCount()
{
  return actionCount;
}

bool SoftwareActionSink::getExecuteLateActions() const
{
  return executeLate;
}

bool SoftwareActionSink::getGenerateDelayed() const
{
  return generateDelayed;
}

void SoftwareActionSink::ResetConflictCount()
{
  ownerOnly();
  conflictCount = 0;
}

void SoftwareActionSink::ResetLateCount()
{
  ownerOnly();
  lateCount = 0;
}

void SoftwareActionSink::ResetDelayedCount()
{
  ownerOnly();
  delayedCount = 0;
}

void SoftwareActionSink::ResetActionCount()
{
  ownerOnly();
  actionCount = 0;
}

void SoftwareActionSink::setExecuteLateActions(bool val)
{
  ownerOnly();
  executeLate = val;
}

void SoftwareActionSink::setGenerateDelayed(bool val)
{
  ownerOnly();
  generateDelayed = val;
}

void SoftwareActionSink::emit(guint64 id, guint64 param, guint64 time, guint64 overtime, gint64 offset, bool late, bool delayed)
{
  bool matched = false;
  for (std::list< Glib::RefPtr<Condition> >::const_iterator condition = conditions.begin(); condition != conditions.end(); ++condition) {
    Glib::RefPtr<SoftwareCondition> softwareCondition =
      Glib::RefPtr<SoftwareCondition>::cast_dynamic(*condition);
    assert(softwareCondition);
    
    // matches?
    if (((softwareCondition->getID() ^ id) & softwareCondition->getMask()) == 0 &&
        softwareCondition->getOffset() == offset) {
      // must happen only one time
      if (matched) clog << kLogErr << "SoftwareActionSiink: single event triggered simultaneous actions on one SoftwareActionSink. Should be impossible!" << std::endl;
      matched = true;
      
      bool conflict = time == lastTime;
      
      if (conflict) {
        ++conflictCount;
        Conflict(conflictCount, lastID, lastParam, lastTime, id, param, time);
      }
      if (delayed) {
        ++delayedCount;
        if (generateDelayed)
          Delayed(delayedCount, id, param, time, overtime);
      }
      if (late) {
        ++lateCount;
        Late(lateCount, id, param, time, overtime);
      }
      ++actionCount;
        
      lastID = id;
      lastParam = param;
      lastTime = time;
      
      if (executeLate || !late) {
        softwareCondition->Action(id, param, time, overtime, late, delayed, conflict);
      }
    }
  }
}

}
