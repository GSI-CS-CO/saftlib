#ifndef TESTPLUGIN_DRIVER1_H_
#define TESTPLUGIN_DRIVER1_H_

#include <MainContext.h>
#include <memory>

#include "Super.h"

namespace testplugin
{
	class Driver1 : public Super
	{
	public:
		Driver1(const std::shared_ptr<Slib::MainContext> &context);
		~Driver1();
		// saftbus method
		bool SayHello();
	private:
		sigc::connection con;
	};

}

#endif
