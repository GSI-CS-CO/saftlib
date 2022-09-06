#include "SAFTd_Proxy.hpp"

#include <saftbus/client.hpp>
#include <memory>

int main() {
	auto saftd = eb_plugin::SAFTd_Proxy::create("/de/gsi/saftlib");

	saftd->AttachDevice("tr0", "dev/ttyUSB0");

	saftbus::SignalGroup::get_global().wait_for_signal();
	return 0;
}