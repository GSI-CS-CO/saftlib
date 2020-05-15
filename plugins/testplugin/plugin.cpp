#include <string>
#include <vector>

extern "C" {
	std::vector<std::string> get_interface_names()
	{
		std::vector<std::string> result;
		result.push_back("Driver1");
		result.push_back("Super");
		return result;
	}
}
