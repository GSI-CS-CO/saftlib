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
	DiceCup::~DiceCup() {
		std::cerr << "DiceCup::~DiceCup" << std::endl;
		// If the DiceCup is destroyed, all Dices contained in it will be destroyed, too.
		// Make sure that the service objects of these Dices are also removed from the service container.
		if (container) {
			for (auto &obj_dice: dices) {
				container->remove_object(obj_dice.first);
			}
		}
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
			std::unique_ptr<Dice6_Service> service (new Dice6_Service(instance.get(), std::bind(&DiceCup::removeDice, this, dice_name)));
			container->create_object(dice_name, std::move(service));
		}
		dices[dice_name] = std::move(instance);
	}

	void DiceCup::addDice12(const std::string &name) {
		std::string dice_name = check_name(name);
		auto instance = std::unique_ptr<Dice12>(new Dice12);
		if (container) {
			std::unique_ptr<Dice12_Service> service (new Dice12_Service(instance.get(), std::bind(&DiceCup::removeDice, this, dice_name)));
			container->create_object(dice_name, std::move(service));
		}
		dices[dice_name] = std::move(instance);
	}

	void DiceCup::removeDice(const std::string &object_path) {
		if (dices.find(object_path) == dices.end()) {
			std::string msg("No dice with name ");
			msg.append(object_path);
			msg.append(" found");
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg);
		}
		dices.erase(object_path);
	}

	Dice* DiceCup::getDice(const std::string &name){
		if (dices.find(name) != dices.end()) {
			return dices[name].get();
		}
		if (name == object_path) {
			return this;
		}
		std::ostringstream msg;
		msg << "no dice with name " << name << " found" << std::endl;
		msg << "available dices: " << std::endl;
		for (auto &dice: dices) {
			msg << "  " << dice.first << std::endl;
		}
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg.str());
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