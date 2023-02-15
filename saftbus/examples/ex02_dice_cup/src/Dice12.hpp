#ifndef DICE12_HPP_
#define DICE12_HPP_

#include "Dice.hpp"

#include <saftbus/loop.hpp>

#include <functional>
#include <memory>

namespace ex02 {

	class Dice12 : public Dice {
	public:
		/// @brief convenience function to allow similar creation compared to Proxy class
		/// @param object_path is ignored
		static std::shared_ptr<Dice12> create(const std::string &object_path);

		/// @brief throw dice once and return the result.
		/// @return one of 1,2,3,4,5,6
		// @saftbus-export
		int throwOnce();
	};


}

#endif
