#include <MainLoop.h>

#include <iostream>

namespace Slib
{

 		void MainLoop::run()
 		{
 			std::cerr << "MainLoop::run()" << std::endl;
 			running = true;
 			while (running) {
	 			std::cerr << "MainLoop: context->iteration(true)" << std::endl;
 				context->iteration(true);
	 			std::cerr << "MainLoop: end if iteration" << std::endl;
 			}
 		}

		void MainLoop::quit()
		{
 			std::cerr << "MainLoop::quit()" << std::endl;
			running = false;
		}

		bool MainLoop::is_running()
		{
			return running;
		}

		std::shared_ptr< MainContext > MainLoop::get_context()
		{
			return context;
		}

		void MainLoop::reference() const
		{
			// nothing
		}

		void MainLoop::unreference() const
		{
			// nothing
		}

		std::shared_ptr< MainLoop > MainLoop::create(bool is_running)
		{
			std::shared_ptr<MainLoop> loop = std::shared_ptr< MainLoop > (new MainLoop);
			loop->context = MainContext::create();
			return loop;
		}

		// std::shared_ptr< MainLoop > MainLoop::create(const std::shared_ptr< MainContext >& context/*, bool is_running*/)
		// {
		// 	return std::shared_ptr< MainLoop > (new MainLoop);
		// }

		int MainLoop::depth()	
		{
			return 1;
		}


}
