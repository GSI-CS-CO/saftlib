#define ETHERBONE_THROWS 1

#include "clog.h"
#include "Display.h"
#include "TimingReceiver.h"
#include "RegisteredObject.h"

namespace saftlib 
{
  Display::Display(ConstructorType args)
  {

  }
  
  Display::~Display()
  {
    
  }
  
  int Display::probe(TimingReceiver* tr, std::map< Glib::ustring,std::map< Glib::ustring, Glib::RefPtr<Owned> > > &otherStuff)
  {
    etherbone::Cycle cycle;
    std::vector<sdb_device> generic_oled;
    std::vector<sdb_device> generic_lcd;
    std::vector<sdb_device> ssd1325;
  
    /* Finds display modules */
    tr->getDevice().sdb_find_by_identity(DISPLAY_GSI_VENDOR_ID, GENERIC_SSD1325_PRODUCT_ID, ssd1325);
    //eb_address_t ssd1325_address = ssd1325[0].sdb_component.addr_first;
  
    
    if (ssd1325.size() == 1)
    {
      clog << kLogDebug << "SSD1325 module loaded!"  << std::endl;
      //int device = GENERIC_SSD1325_PRODUCT_ID;

      //std::ostringstream path;
      //path.imbue(std::locale("C")); // Avoid commas in numbers
      //path << od.objectPath << "/displays/ssd1325";
      //std::ostringstream name;
      //name.imbue(std::locale("C"));
      //name << "ssd1325-" << device;
      //
      //Display::ConstructorType args = { tr.operator->(), device_address };
      //Glib::RefPtr<Display> Display = Display::create(Glib::ustring(path.str()), args);
      //tr->otherStuff["Display"][name.str()] = Display;
      
      //if (ioctl.size() == 1) {
      //  int device = 10;
      //  unsigned int device_address = (eb_address_t)ioctl[0].sdb_component.addr_first;
      //  
      //  std::ostringstream path;
      //  path.imbue(std::locale("C")); // Avoid commas in numbers
      //  path << od.objectPath << "/ioctl";
      //  std::ostringstream name;
      //  name.imbue(std::locale("C"));
      //  name << "ioctl-" << device;
      //  
      //  IOControl::ConstructorType args = { tr.operator->(), device_address };
      //  Glib::RefPtr<IOControl> IOControl = IOControl::create(Glib::ustring(path.str()), args);
      //  tr->otherStuff["IOControl"][name.str()] = IOControl;
      //}
      //else {
      //  // !!! TBD: What is the best way to print a warning here? throw Gio::DBus? clog << kLogDebug?
      //}
      
      
    }
  
    return 0;
  }
} /* namespace saftlib */


