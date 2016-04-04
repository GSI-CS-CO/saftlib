#ifndef OWNED_H
#define OWNED_H

#include "interfaces/iOwned.h"
#include "BaseObject.h"

namespace saftlib {

class Owned : public BaseObject, public iOwned
{
  public:
    Owned(const Glib::ustring& objectPath, sigc::slot<void> destroy_ = sigc::slot<void>());
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
    
  protected:
    virtual void ownerQuit();
    static void owner_quit_handler(
      const Glib::RefPtr<Gio::DBus::Connection>&,
      const Glib::ustring&, const Glib::ustring&, const Glib::ustring&,
      const Glib::ustring&, const Glib::VariantContainerBase&, Owned* self);
  
  private:
    sigc::slot<void> destroy;
    sigc::slot<void> unsubscribe;
    Glib::ustring owner;
};

}

#endif
