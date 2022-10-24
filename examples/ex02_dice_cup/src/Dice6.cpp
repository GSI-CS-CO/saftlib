#include "Dice6.hpp"

#include <saftbus/error.hpp>

namespace ex02 {

	std::shared_ptr<Dice6> Dice6::create(const std::string &object_path) {
		return std::make_shared<Dice6>();
	}

	int Dice6::throwOnce() {
		int result = rand()%6+1;
		if (was_thrown) {
			was_thrown(result);
		}
		return result;
	}

}