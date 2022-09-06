#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

namespace eb_plugin {

	SAFTd::SAFTd(saftbus::Container *c, const std::string &obj_path) 
		: container(c)
		, object_path(obj_path)
	{
	}

	std::string SAFTd::AttachDevice(const std::string& name, const std::string& path) {
		std::string result = object_path;
		result.append("/");
		result.append(name);

		std::unique_ptr<SAFTd>         instance(new SAFTd(container, result));
		std::unique_ptr<SAFTd_Service> service (new SAFTd_Service(std::move(instance)));

		container->create_object(result, std::move(service));

		return result;
	}
	std::string SAFTd::EbForward(const std::string& saftlib_device) {
		return std::string();
	}
	void SAFTd::RemoveDevice(const std::string& name) {

	}
	void SAFTd::Quit() {

	}
	std::string SAFTd::getSourceVersion() const {
		return std::string();

	}
	std::string SAFTd::getBuildInfo() const {
		return std::string();

	}
	std::map< std::string, std::string > SAFTd::getDevices() const {
		return std::map< std::string, std::string >();
	}


}