/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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

#include "TimingReceiver.hpp"
#include "SAFTd.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"
#include "IoControl.hpp"

#include <saftbus/error.hpp>

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>






namespace eb_plugin {

TimingReceiver::TimingReceiver(SAFTd &saftd, const std::string &n, const std::string &eb_path, saftbus::Container *container)
	: OpenDevice(saftd.get_etherbone_socket(), eb_path)
	, WhiteRabbit(OpenDevice::device)
	, Watchdog(OpenDevice::device)
	, ECA(saftd, OpenDevice::device, object_path, container)
	, BuildIdRom(OpenDevice::device)
	, TempSensor(OpenDevice::device)
	, io_control(OpenDevice::device)
	, object_path(saftd.get_object_path() + "/" + n)
	, name(n)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;

	if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
	}
	
	// couple the IoControls to ECA channel 0 and event source


	poll(); // update locked status ...
	//    ... and repeat every 1s 
	poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&TimingReceiver::poll, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);
}



TimingReceiver::~TimingReceiver() 
{
	std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;
	std::cerr << "saftbus::Loop::get_default().remove(poll_timeout_source)" << std::endl;
	saftbus::Loop::get_default().remove(poll_timeout_source);
}




bool TimingReceiver::poll()
{
	std::cerr << "TimingReceiver::poll()" << std::endl;
	WhiteRabbit::getLocked();
	Watchdog::update(); 
	return true;
}


const std::string &TimingReceiver::get_object_path() const
{
	return object_path;
}

void TimingReceiver::Remove() {
	throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver::Remove is deprecated, use SAFTd::Remove instead");
}

std::string TimingReceiver::getName() const
{
	return name;
}

eb_plugin::Time TimingReceiver::CurrentTime()
{
	if (!WhiteRabbit::locked) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver is not Locked");
	}
	return eb_plugin::makeTimeTAI(ReadRawCurrentTime());
}

void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time)
{
	ECA::InjectEventRaw(event, param, time.getTAI());
}


} // namespace saftlib


