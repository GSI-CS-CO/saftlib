#ifndef SAFTLIB_TIMING_RECEIVER_ADDON_HPP_
#define SAFTLIB_TIMING_RECEIVER_ADDON_HPP_

#include <string>
#include <map>

namespace saftlib {

	class TimingReceiverAddon
	{
	public:
		TimingReceiverAddon();
		virtual ~TimingReceiverAddon();

		virtual std::map< std::string, std::map< std::string, std::string> > getObjects() = 0;
	};



}

#endif
