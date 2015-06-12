#ifndef OWNED_H
#define OWNED_H

#include "interfaces/iOwned.h"

namespace saftlib {

class Owned : public iOwned
{
  public:
    Owned(sigc::slot<void> destroy_ = sigc::slot<void>());
    
    void Disown();
    void Own();
    void Destroy();
    Glib::ustring getOwner() const;
    bool getDestructible() const;
    
    // use this at the start of protected methods
    void owner_only() const;
    
    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::ustring& getObjectPath() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
  
  private:
    sigc::slot<void> destroy;
    Glib::ustring owner;
};

}

#endif
