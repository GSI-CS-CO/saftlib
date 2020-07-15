#include "Driver1.h"

#include <iostream>

namespace testplugin
{

		Driver1::Driver1(const std::shared_ptr<Slib::MainContext> &context) 
		{
			std::cerr << "Driver1::Driver1" << std::endl;
			con = context->connect(sigc::mem_fun(this, &Driver1::SayHello), 1000);
		}

		Driver1::~Driver1() {
			std::cerr << "Driver1::~Driver1" << std::endl;
			con.disconnect();
		}

		bool Driver1::SayHello() {
			std::cout << "Hello from Driver1" << std::endl;
			return true;
		}





}
