/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#include <MainLoop.h>

#include <iostream>

namespace Slib
{

 		void MainLoop::run()
 		{
 			running = true;
 			while (running) {
 				context->iteration(true);
 			}
 		}

		void MainLoop::quit()
		{
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
