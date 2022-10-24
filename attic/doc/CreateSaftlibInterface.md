# How to create a Saftlib Interface

## Recipe
1. Get the requirements from the user and make sure you provide (only) the necessary methods and properties.
2. Create an interface (XML) file (saftlib/interfaces).
3. Add the interface file to the makefile (saftlib/interfaces/makefile.am).
4. Implement the driver (saftlib/drivers).
5. Add the driver files to the top makefile (saftlib/makefile.am).

## How to create an interface file
`<interface name="de.gsi.saftlib.InterfaceName>`

An interface can have methods, signals and properties.

## Methods
`<method name="MethodName">`

Methods in an interface are called by a client (via a proxy object) and executed by the driver.

A method has a number of arguments

### Arguments
`<arg direction = "in" name = "ArgName" type="x">`

* One argument may have direction = "out" specifying the return value
* Other arguments must have direction = "in" specifying the parameters
* argument types use the dbus standard single-letter codes: 
<https://dbus/freedesktop.org/doc/dbus-specification.html#type-system>


`i -> int32`

`ai -> vector<int32>`

`aai -> vector<vector<int32>>`

`a{is} -> map<int32,string>`

`a{ia{su}} -> map<int32,map{string,uint32>>`

`v -> Glib::VariantBase`

`tuples (ii) not supported`

### Naming
* If a Method issues a write or a read, please call it WriteMethodName or ReadMethodName.

## Signals
`<signal name="SignalName">`

Signals are created by the driver and received by any clients that have registered a callback on that signal.

### Arguments
Signal arguments are similar to method arguments but have no direction (always driver->client)

## Properties
`<property name="PropertyName" type="x" access="read">`

Properties are variables stored by both the driver and proxy. The proxy copy of the variable is a cached copy of that in the driver, updated via a signal. Behaviour depends on the access type

* Const proprieties are initialized at start-up and they never change. No signal is created.
* Read properties are initialized at start-up and then updated on every PropertyName signal
* Readwrite properties are initialized at start-up and then updated on every PropertyName signal. When written, the proxy copy is not updated until a signal is received
* Please avoid write-only properties. This is not supported, expose a write method instead.

### Annotations
Property behaviour is modified by annotations

`<annotation name="org.freedesktop.DBus.Property.EmitsChangedSignal" value="const"/>`
* const: constant, no signal created
* true: variable, signal created, signal sent automatically after change
* false: variable, no signal generated

### Signals and update behaviour
When reading a property from a proxy the cached copy will be read. The cached copy will only be refreshed when a signal is received by a running mainloop. Be aware that constantly polling a property in a loop will not allow the mainloop to process incoming events. Properties are thus updated after the method in which they were changed; if immediate feedback is required from an action use a method return value, not a property.

### Naming
* The driver must provide a read method named getPropertyName
* read/write properties must also provide a write method named setPropertyName
* a signal named PropertyName is automatically generated for non-const properties 
