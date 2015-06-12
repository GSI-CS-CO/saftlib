#ifndef OWNED_H
#define OWNED_H

#include "interfaces/iOwned.h"

namespace saftlib {

class Owned : public iOwned
{
  public:
    Owned(sigc::slot<void> destroy_ = sigc::slot<void>());
    ~Owned();
    
    void Disown();
    void Own();
    void Destroy();
    Glib::ustring getOwner() const;
    bool getDestructible() const;
    
    // use this at the start of protected methods
    void ownerOnly() const;
    // only use this immediately after object creation
    void initOwner(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& owner);
    
    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::ustring& getObjectPath() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
  
  protected:
    virtual void owner_quit_handler(
      const Glib::RefPtr<Gio::DBus::Connection>&,
      const Glib::ustring&, const Glib::ustring&, const Glib::ustring&,
      const Glib::ustring&, const Glib::VariantContainerBase&);
  
  private:
    sigc::slot<void> destroy;
    sigc::slot<void> unsubscribe;
    Glib::ustring owner;
};

}

#endif
