#include "DiceCup.hpp"
#include "Dice6.hpp"
#include "Dice12.hpp"

#include "Dice6_Service.hpp"
#include "Dice12_Service.hpp"

#include <saftbus/error.hpp>

namespace ex02 {

	std::shared_ptr<DiceCup> DiceCup::create(const std::string &object_path) {
		return std::make_shared<DiceCup>(object_path);
	}


	DiceCup::DiceCup(const std::string &obj_path, saftbus::Container *cont) 
		: Dice()
		, container(cont)
		, object_path(obj_path)
	{
	}

	std::string DiceCup::check_name(const std::string &name) {
		std::string dice_name(object_path);
		dice_name.append("/");
		dice_name.append(name);
		if (dices.find(dice_name) != dices.end()) {
			std::string msg("A dice with name ");
			msg.append(dice_name);
			msg.append(" exists already");
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg);
		}
		return dice_name;
	}

	void DiceCup::addDice6(const std::string &name) {
		std::string dice_name = check_name(name);
		auto instance = std::unique_ptr<Dice6>(new Dice6);
		if (container) {
			std::unique_ptr<Dice6_Service> service (new Dice6_Service(instance.get()));
			container->create_object(dice_name, std::move(service));
		}
		dices[dice_name] = std::move(instance);
	}

	void DiceCup::addDice12(const std::string &name) {
		std::string dice_name = check_name(name);
		auto instance = std::unique_ptr<Dice12>(new Dice12);
		if (container) {
			std::unique_ptr<Dice12_Service> service (new Dice12_Service(instance.get()));
			container->create_object(dice_name, std::move(service));
		}
		dices[dice_name] = std::move(instance);
	}

	Dice* DiceCup::getDice(const std::string &name){
		if (dices.find(name) != dices.end()) {
			return dices[name].get();
		}
		if (name == object_path) {
			return this;
		}
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such dice");
	}


	int DiceCup::throwOnce() {
		int result = 0;
		for(auto &dice: dices) {
			result += dice.second->throwOnce();
		}
		if (was_thrown) {
			was_thrown(result);
		}
		return result;
	}


}