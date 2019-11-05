#ifndef SAFTLIB_EB_FORWARD_H
#define  SAFTLIB_EB_FORWARD_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "giomm.h"
#include "MainContext.h"


namespace saftlib {

	class EB_Forward {
	public:
		EB_Forward(const std::string &device_name, const std::string & eb_name); 
		~EB_Forward();

		bool accept_connection(Slib::IOCondition condition);
		void wait_for_client(); 
		void close_connection();


	private:
		int     _eb_device_fd, _pts_fd; 
		uint8_t _write_buffer[32768];
		int     _write_buffer_length;
	};


}

#endif
