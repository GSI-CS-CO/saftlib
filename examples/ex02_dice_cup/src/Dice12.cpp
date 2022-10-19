#include "Dice12.hpp"

#include <saftbus/error.hpp>

namespace ex02 {

	std::shared_ptr<Dice12> Dice12::create(const std::string &object_path) {
		return std::make_shared<Dice12>();
	}

	int Dice12::throwOnce() {
		int result = rand()%12+1;
		if (was_thrown) {
			was_thrown(result);
		}
		return result;
	}

}