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
		EB_Forward(const std::string& eb_name); 
		~EB_Forward();

		bool accept_connection(Slib::IOCondition condition);

		std::string saft_eb_devide();

	private:
		void write_all(int fd, char *ptr, int size);
		void read_all(int fd, char *ptr, int size);
		void open_pts();
		int     _eb_device_fd, _pts_fd; 
	};


}

#endif
