#include "eb-forward.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

namespace saftlib {

	EB_Forward::EB_Forward(const std::string & eb_name)
	{
		_eb_device_fd = open(eb_name.c_str(), O_RDWR);
		if (_eb_device_fd == -1) {
			return;
		}

		_pts_fd = open("/dev/ptmx", O_RDWR);
		grantpt(_pts_fd);
		unlockpt(_pts_fd);

		Slib::signal_io().connect(sigc::mem_fun(*this, &EB_Forward::accept_connection), _pts_fd, Slib::IO_IN, Slib::PRIORITY_LOW);
	} 
	EB_Forward::~EB_Forward()
	{
		close(_eb_device_fd);		
	}

	bool EB_Forward::accept_connection(Slib::IOCondition condition)
	{
		static std::vector<uint8_t> request;  // data from eb-tool
		static std::vector<uint8_t> response; // data from device
		request.clear();
		response.clear();

		unsigned request_size = 0;

		for (;;) {
			uint8_t val;
			int result = read(_pts_fd, (void*)&val, sizeof(val));
			if (result == 0) { // end of file 
				return false;
			}
			if (result == -1) { // error
				std::cerr << "EB_Forward error(" << std::dec << errno << "): " << strerror(errno) << std::endl;
				return true;
			}
			if (result == sizeof(val)) { // normal read 
				request.push_back(val);

				if (request.size() == 4) {
					if (request[0]     == 0x4e && // test for Etherbone magic word
						request[1]     == 0x6f &&
						request[2]     == 0x11 &&
						request[3]     == 0xff) {

						response.push_back(0x4e);
						response.push_back(0x6f);
						response.push_back(0x16);
						response.push_back(0x44);

						for (int i = 0; i < 4; ++i) {
							read(_pts_fd, (void*)&val, sizeof(val));
							response.push_back(val);
						}
						write(_pts_fd, (void*)&response[0], response.size());
						// handling of Etherbone header done.
						return true;
					} else { // assume it is an Etherbone cycle header, calculate the response size
						int wcount = request[2];
						int rcount = request[3];
						request_size += 4; // for the cycle header
						if (wcount) {
							request_size += 4 + 4*wcount; // base address + wcount write values 
						}
						if (rcount) {
							request_size += 4 + 4*rcount; // base return address + rcount read addresses
						}
					}
				}

				// process the request by sending it to the device,
				//  read the response from the device and write the 
				//   response back to the eb-tool
				if (request_size && request_size == request.size()) {
					// all cycle bytes read
					write(_eb_device_fd, (void*)&request[0], request.size());
					response.clear();
					response.resize(request.size());
					read(_eb_device_fd, (void*)&response[0], response.size());
					write(_pts_fd, (void*)&response[0], response.size());
					return true;
				}

			}
		}
		return false;
	}	


	std::string EB_Forward::saft_eb_devide()
	{
		return std::string(ptsname(_pts_fd));
	}


}