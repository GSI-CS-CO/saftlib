## How to Create a Saftlib Driver

## Ingredients
`interfaces/InterfaceName.xml`

Your interface definition

`interfaces/InterfaceName.h`

Generated Proxy object for client, Service for driver

`interfaces/iInterfaceName.h`
`interfaces/iInterfaceName.cpp`

Generated code to manage connection to dbus (definition of iInterfaceName_Proxy, iInterfaceName_Service)

Generated definition of iInterfaceName, abstract class.

## Tasks

### Automake

Create drivers/InterfaceName.h|.cpp and add to automake

### Implement the Interface

`\#include "interface/Interfacename.h"`

`class InterfaceName : public iInterfaceName, public Glib::Object ...`

and implement the abstract methods:

`T1 MethodName(T2,T3,...);`

For Properties:

`T getPropertyName() const;`

`void setPropertyName(T);` for readwrite

### Create create function
To link the driver and the generated service code the driver is insantiated via the function:

    `Glib::RefPtr<InterfaceName> create(const Glib::ustring& objectPath, ConstructorType args) {
      return RegisteredObject<InterfaceName>::create(objectPath, args);
    }`

this now requires
* `typedef InterfaceName_Service ServiceType`
* `struct ConstructorType {...};`
* constructor signature `InterfaceName(ConstructorType)`

### Use of Signals
Signals:

SignalName(arg1, arg2);

Signalling changes to read/readwrite properties. A signal with the same name as the property is automatically generated:

PropertyName(newPropertyValuevalue);
