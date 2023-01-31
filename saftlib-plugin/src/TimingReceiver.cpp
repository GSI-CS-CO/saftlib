/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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
#include "io_control_regs.h"
#include "Output.hpp"
#include "Output_Service.hpp"
#include "Input.hpp"
#include "Input_Service.hpp"

#include <saftbus/error.hpp>

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>


namespace saftlib {

TimingReceiver::TimingReceiver(SAFTd &saftd, const std::string &n, const std::string &eb_path, int polling_interval_ms, saftbus::Container *cont)
	: OpenDevice(saftd.get_etherbone_socket(), eb_path, polling_interval_ms, &saftd)
	, Watchdog(OpenDevice::device)
	, WhiteRabbit(OpenDevice::device)
	, ECA(saftd, OpenDevice::device, saftd.getObjectPath() + "/" + n, cont)
	, ECA_TLU(OpenDevice::device, cont)
	, ECA_Event(OpenDevice::device, cont)
	, BuildIdRom(OpenDevice::device)
	, TempSensor(OpenDevice::device)
	, Reset(OpenDevice::device)
	, Mailbox(OpenDevice::device)
	, LM32Cluster(OpenDevice::device, this)
	, container(cont)
	, io_control(OpenDevice::device)
	, object_path(saftd.getObjectPath() + "/" + n)
	, name(n)
{
	// std::cerr << "TimingReceiver::TimingReceiver" << std::endl;

	if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
	}
	
	unsigned eca_channel_for_outputs = 0; // ECA channel 0 is always for IO

	// attach Outputs to ECA and Inputs to ECA_TLU
	auto &ios = io_control.get_ios();
	for(auto &io: ios) {
		// the object paths need to be created before creating Input and Output objects, 
		// because Input needs Output-Partner-Path and vice versa.
		std::string input_path;
		std::string output_path;
		if (io.getDirection() == IO_CFG_FIELD_DIR_INPUT  || io.getDirection() == IO_CFG_FIELD_DIR_INOUT) {
			input_path.append(object_path);
			input_path.append("/inputs/");
			input_path.append(io.getName());
		}
		if (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT) {
			output_path.append(object_path);
			output_path.append("/outputs/");
			output_path.append(io.getName());
		}

		// create inputs
		if (io.getDirection() == IO_CFG_FIELD_DIR_INPUT  || io.getDirection() == IO_CFG_FIELD_DIR_INOUT) {
			std::unique_ptr<Input> input(new Input(*dynamic_cast<ECA_TLU*>(this), input_path, output_path, 
												   io.getIndexIn(), &io, container));
			if (container) {
				std::unique_ptr<Input_Service> service(new Input_Service(input.get()));
				container->create_object(input_path, std::move(service));
			}
			ECA_TLU::addInput(std::move(input));
		}

		// create output
		if (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT) {
			std::unique_ptr<Output> output(new Output(*dynamic_cast<ECA*>(this), io, output_path, input_path, 
													  eca_channel_for_outputs, container));
			if (container) {
				std::unique_ptr<Output_Service> service(new Output_Service(output.get()));
				container->create_object(output_path, std::move(service));
			}
			ECA::addActionSink(eca_channel_for_outputs, std::move(output));
		}
	}

	poll(); // update locked status ...
	//    ... and repeat every 1s 
	poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&TimingReceiver::poll, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);
}

TimingReceiver::~TimingReceiver() 
{
	// std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;
	// std::cerr << "saftbus::Loop::get_default().remove(poll_timeout_source)" << std::endl;
	saftbus::Loop::get_default().remove(poll_timeout_source);

	// remove the service objects for all addons
	if (container != nullptr) {
		for (auto &addon: addons) {
			for (auto &object: addon.second->getObjects()) {

				try {
					container->remove_object(object.second);
					// container->remove_object(actionSink->getObjectPath());
				} catch (saftbus::Error &e) {
					// std::cerr << "removal attempt failed: " << e.what() << std::endl;
				}

			}
		}
	}
}

bool TimingReceiver::poll()
{
	// std::cerr << "TimingReceiver::poll()" << std::endl;
	WhiteRabbit::getLocked();
	Watchdog::update(); 
	Reset::WdRetrigger();
	return true;
}


const std::string &TimingReceiver::getObjectPath() const
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

saftlib::Time TimingReceiver::CurrentTime() const
{
	if (!WhiteRabbit::locked) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver is not Locked");
	}
	return saftlib::makeTimeTAI(ReadRawCurrentTime());
}

void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, saftlib::Time time) const
{
	// std::cerr << "TimingReceiver::InjectEvent" << std::endl;
	ECA_Event::InjectEventRaw(event, param, time.getTAI());
}

std::map< std::string, std::map< std::string, std::string > > TimingReceiver::getInterfaces() const
{
	std::map< std::string, std::map< std::string, std::string > > result;
	
	result["SoftwareActionSink"]    = ECA::getSoftwareActionSinks();
	result["SCUbusActionSink"]      = ECA::getSCUbusActionSinks();
	result["WbmActionSink"]         = ECA::getWbmActionSinks();
	result["EmbeddedCPUActionSink"] = ECA::getEmbeddedCPUActionSinks();
	result["Output"]                = ECA::getOutputs();
	result["Input"]                 = ECA_TLU::getInputs();
	for (auto &addon: addons) {
		for (auto &object: addon.second->getObjects()) {
			result[addon.first][object.first] = object.second;
		}
	}
	
	return result;
}


// void TimingReceiver::addInterfaces(const std::string &interface_name, const std::map< std::string, std::string > & objects)
// {
// 	for(auto &object: objects) {
// 		addon_interfaces[interface_name][object.first] = object.second;
// 	}
// }

void TimingReceiver::installAddon(const std::string &interface_name, std::unique_ptr<TimingReceiverAddon> addon)
{
	addons[interface_name] = std::move(addon);
}

void TimingReceiver::removeAddon(const std::string &interface_name) {
	std::cerr << "TimingReceiver::removeAddon(" << interface_name << ")" << std::endl;
	addons.erase(interface_name);
} 



} // namespace saftlib


