# saftbus-gen: A code generator for saftbus Service and Proxy classes

saftbus-gen is a companion tool for [saftbus](../saftbus/README.md)
saftbus-gen takes as input a C++ source file with class declarations. 
It follows #include directives when the filename is written in double quotes.
```C++
#include <will_not_follow.h>
#include "will_follow.h"
```
In case a compiler switch such as (-I /include/directory) is needed for the compiler find an include file, the same switch needs to be passed to saftbus-gen:

    saftbus-gen -I /include/directory MyClass.hpp

While reading the C++ source files, saftbus-gen will track namespaces and identify class declarations.
Be aware, that saftbus-gen does not contain the full C++ grammar. You may encounter situations where it misinterprets the code. It is recommended to always check the output of saftbus-gen if the correct function and signal signatures are generated.
Whenever it sees a special comment (// @saftbus-export) above of a class method declaration or a class member variable of type sigc::signal<> or std::function<>, it will generate four C++ source files for the class.
Suppose the name of the class was ClassName, it will generate these files:
  - ClassName_Service.hpp
  - ClassName_Service.cpp
  - ClassName_Proxy.hpp
  - ClassName_Proxy.cpp

At the moment, all classes with // @saftbus-export annotations need to be inside of a namespace, not in global scope.
The proxy methods will properly work if all function arguments are one of 
  - simple types (int, double, POD-structs, ...)
  - std::string
  - std::vector\<simple type\>
  - std::vector\<std::string\>
  - nested vectors
  - maps of the above mentioned types
  - complex types if they derive from saftbus::SerDesAble

If user defined types are used as function arguments, then the type declaration should be in an external #include file and the #include directive should be annotated with // @saftbus-export. 
Then, saftbus-gen will copy that #include directive into the generated source file.
If a user defined type is not a PDO-struct, it must derive from saftbus::SerDesAble.

## Summary
Annotations (// @saftbus-export) can appear
  - above #include directives
  - above method declarations in classes
  - above member variable declarations of type sigc::signal or std::function
```C++
#include "BaseClass.hpp"
// @saftbus-export
#include <my_special_type.h>

namespace my_namespace {
	class MyClass : public BaseClass {
	public:
		// @saftbus-export
		int do_something(my_special_type data, int id);
		// @saftbus-export
		sigc::signal<void, int> something_happened;
	};
}
```