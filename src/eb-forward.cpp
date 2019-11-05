#include "eb-forward.h"

#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>

namespace saftlib {

	EB_Forward::EB_Forward(const std::string & device_name, const std::string & eb_name)
	{
		_eb_device_fd = open(eb_name.c_str(), O_RDWR | O_NONBLOCK );
		if (_eb_device_fd == -1) {
			return;
		}
		_write_buffer_length = 0;
	} 
	EB_Forward::~EB_Forward()
	{
	}

	bool EB_Forward::accept_connection(Slib::IOCondition condition)
	{
		uint32_t header1;
		uint32_t header1_response = 0x44166f4e;
		uint32_t header2;


		read(_pts_fd, (void*)&header1, sizeof(header1));
		write(_pts_fd, &header1_response, sizeof(header1_response));
		if (header1 == 0xff116f4e) {
			std::cerr << "accept_connection_h1 0x" << std::setfill('0') << std::setw(8) << std::hex << header1 << std::endl;
			read(_pts_fd, (void*)&header2, sizeof(header2));
			write(_pts_fd, &header2, sizeof(header2));
		} else {
			std::cerr << "header1 0x" << std::setfill('0') << std::setw(8) << std::hex << header1 << std::endl;
		}

		for (;;) {
			uint8_t val;
			int result;
			int n = 0;
			while ((result = read(_pts_fd, (void*)&val, sizeof(val))) == sizeof(val)) {
				++n;
				std::cerr<< "<<<< 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)val << std::endl;
				write(_eb_device_fd, (void*)&val, sizeof(val));
			}
			if (result == -1) {
				std::cerr << errno << " " << strerror(errno) << std::endl;
			} 
			if (result == 0) {
				std::cerr << "transfer done" << std::endl;
				_write_buffer_length = 0;
				return true;
			}
			while (n > 0) {
				if ( read(_eb_device_fd, (void*)&val, sizeof(val)) == sizeof(val)) {
					std::cerr<< ">>>>(" << std::dec << n << ") 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)val << std::endl;
					_write_buffer[_write_buffer_length++] = val;
					--n;
				}
			}
			write(_pts_fd, (void*)_write_buffer, _write_buffer_length);
			_write_buffer_length = 0;
		}
		return true; // MainLoop should stop watching _create_socket file descriptor		
	}
	void EB_Forward::wait_for_client()
	{
		std::cerr << "EB_Forward::wait_for_client()" << std::endl;
		_pts_fd = open("/dev/pts/24", O_RDWR | O_NONBLOCK);
		try {
			Slib::signal_io().connect(sigc::mem_fun(*this, &EB_Forward::accept_connection), _pts_fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
		} catch(std::exception &e)	{
			throw;
		}
	} 
	void EB_Forward::close_connection()
	{
		close(_eb_device_fd);
	}


}