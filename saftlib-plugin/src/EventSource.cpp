#include "EventSource.hpp"

namespace saftlib {

	EventSource::EventSource(const std::string &obj_path, const std::string &name, saftbus::Container *container) 
		: Owned(container)
		, object_path(obj_path)
		, object_name(name)
	{
	}

	std::string EventSource::getObjectPath() const {
		return object_path;
	}

	std::string EventSource::getObjectName() const {
		return object_name;
	}


}