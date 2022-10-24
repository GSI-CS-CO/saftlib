#include "eb-forward.hpp"

#include <saftbus/loop.hpp>

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include <iostream>
#include <iomanip>
#include <functional>

namespace eb_plugin {

	void EB_Forward::open_pts() 
	{
		_pts_fd = open("/dev/ptmx", O_RDWR);
		grantpt(_pts_fd);
		unlockpt(_pts_fd);
		chmod(ptsname(_pts_fd), S_IRUSR | S_IWUSR | 
			                    S_IRGRP | S_IWGRP | 
			                    S_IROTH | S_IWOTH );

		std::cerr << "eb-forward " << eb_forward_path() << std::endl;
		//Slib::signal_io().connect(sigc::mem_fun(*this, &EB_Forward::accept_connection), _pts_fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
		saftbus::Loop::get_default().connect<saftbus::IoSource>(std::bind(&EB_Forward::accept_connection, this, std::placeholders::_1), _pts_fd, POLLIN);
	}

	EB_Forward::EB_Forward(const std::string& eb_name)
	{	
		if (eb_name.size()) {
			if (eb_name[0] == '/') {
				_eb_device_fd = open(eb_name.c_str(), O_RDWR);
			} else {
				std::string name = "/";
				name.append(eb_name);
				_eb_device_fd = open(name.c_str(), O_RDWR);
			}
			if (_eb_device_fd > 0) {
				open_pts();
				return;
			}
		}
		open_pts();
	} 
	EB_Forward::~EB_Forward()
	{
		close(_eb_device_fd);		
	}

	bool EB_Forward::accept_connection(int condition)
	{
		std::cerr << "EB_Forward::accept_connection" << std::endl;
		static std::vector<uint8_t> request;  // data from eb-tool
		static std::vector<uint8_t> response; // data from device
		request.clear();
		response.clear();

		unsigned request_size = 0;

		for (;;) {
			uint8_t val;
			int result = read(_pts_fd, (void*)&val, sizeof(val));
			if (result <= 0) { // end of file 
				close(_pts_fd); // close and reopen immediately
				open_pts(); 
				return false; // remove old fd from loop
			}
			if (result == sizeof(val)) { // normal read 
				request.push_back(val);

				if (request.size() == 4) {
					if ( request[0]     == 0x4e  // test for Etherbone magic word
					  && request[1]     == 0x6f 
					  && request[2]     == 0x11 
					  )//&& (request[3]     == 0xff || request[3] == 0x77)) // on 32 bit systems, the host will send 0x77 while on 64 bit systems the host will send 0xff
					{
						// hard-coded response
						response.push_back(0x4e);
						response.push_back(0x6f);
						response.push_back(0x16);
						response.push_back(0x44);

						for (int i = 0; i < 4; ++i) {
							result = read(_pts_fd, (void*)&val, sizeof(val));
							if (result <= 0) { 
								close(_pts_fd); // close and reopen immediately
								open_pts(); 
								return false; // remove old fd from loop
							}
							response.push_back(val);
						}
						result = write(_pts_fd, (void*)&response[0], response.size());
						if (result <= 0) { 
							close(_pts_fd); // close and reopen immediately
							open_pts(); 
							return false; // remove old fd from loop
						}
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
					write_all(_eb_device_fd, (char*)&request[0], request.size());
					response.clear();
					response.resize(request.size());
					struct pollfd pfds[1] = {0,};
					pfds[0].fd = _eb_device_fd;
					pfds[0].events = POLLIN | POLLHUP;
					if (poll(pfds,1,-1) == 0) {
						return false;
					}
					read_all(_eb_device_fd, (char*)&response[0], response.size());
					write_all(_pts_fd, (char*)&response[0], response.size());
					return true;
				}

			}
		}
		return false;
	}	

	void EB_Forward::write_all(int fd, char *ptr, int size) 
	{
		while (size > 0) {
			int result = write(fd, ptr, size);
			if (result > 0) {
				size -= result;
				ptr += result;
			} else {
				// error... very bad... dont know how to handle this 
				// this probably means that the device is not connected anymore
				return;
			}
		}
	}
	void EB_Forward::read_all(int fd, char *ptr, int size) 
	{
		while (size > 0) {
			int result = read(fd, ptr, size);
			if (result > 0) {
				size -= result;
				ptr += result;
			} else {
				// error... very bad... dont know how to handle this 
				// this probably means that the device is not connected anymore
				return;
			}
		}
	}

	std::string EB_Forward::eb_forward_path()
	{
		return std::string(ptsname(_pts_fd));
	}

}

