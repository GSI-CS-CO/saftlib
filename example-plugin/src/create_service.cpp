#include "SpecialDice_Service.hpp"
#include <saftbus/make_unique.hpp>

extern "C" std::unique_ptr<saftbus::Service> create_service() {
	return std2::make_unique<example::SpecialDice_Service>();
}
