#ifndef MY_TYPE_HPP_
#define MY_TYPE_HPP_

#include <saftbus/saftbus.hpp>
#include <string>

namespace ex01 {

	struct MyType : public saftbus::SerDesAble {
		std::string type;
		std::string name;
		void serialize(saftbus::Serializer &ser) const {
			ser.put(type);
			ser.put(name);
		}
		void deserialize(const saftbus::Deserializer &des) {
			des.get(type);
			des.get(name);
		}
	};


}


#endif
