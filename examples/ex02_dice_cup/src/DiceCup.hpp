#ifndef DICE_CUP_HPP_
#define DICE_CUP_HPP_

#include "Dice.hpp"

#include <saftbus/loop.hpp>
#include <saftbus/server.hpp>

#include <functional>
#include <string>
#include <memory>
#include <map>

namespace ex02 {


	class DiceCup : public Dice {
		saftbus::Container *container;
		std::string object_path;
		std::map<std::string, std::unique_ptr<Dice> > dices;
		std::string check_name(const std::string &name);
	public:
		/// @brief convenience function to allow similar creation compared to Proxy class
		/// @param object_path is ignored
		static std::shared_ptr<DiceCup> create(const std::string &object_path);

		DiceCup(const std::string &object_path, saftbus::Container *container = nullptr);
		
		Dice* getDice(const std::string &name);

		// @saftbus-export
		void addDice6(const std::string &name);

		// @saftbus-export
		void addDice12(const std::string &name);

		/// @brief throw dice once and return the result.
		/// @result the sum of all dices in the cup
		// @saftbus-export
		int throwOnce();

	};


}

#endif
