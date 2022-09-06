#ifndef TR_SAFTD_HPP_
#define TR_SAFTD_HPP_

#include <saftbus/loop.hpp>
#include <saftbus/service.hpp>

#include <memory>
#include <string>
#include <map>

#include <sys/stat.h>

#include "Device.hpp"
#include "eb-forward.hpp"

namespace eb_plugin {

	class SAFTd {
	public:
		SAFTd(saftbus::Container *c, const std::string &obj_path);
		// @saftbus-export
		std::string AttachDevice(const std::string& name, const std::string& path);
		// @saftbus-export
		std::string EbForward(const std::string& saftlib_device);
		// @saftbus-export
		void RemoveDevice(const std::string& name);
		// @saftbus-export
		void Quit();
		// @saftbus-export
		std::string getSourceVersion() const;
		// @saftbus-export
		std::string getBuildInfo() const;
		// @saftbus-export
		std::map< std::string, std::string > getDevices() const;
	private:
		saftbus::Container *container;
		std::string object_path;
	    etherbone::Socket socket;
	    std::map< std::string, std::unique_ptr<EB_Forward> > m_eb_forward; 
	    saftbus::Loop *m_loop;
	    saftbus::Source *eb_source;
	    saftbus::Source *msi_source;
	    
	    std::map< std::string, Device > devs;

	    // remember the mode of a device file
	    std::map< std::string, std::pair<std::string, struct stat > > dev_stats;

	};


}

#endif

