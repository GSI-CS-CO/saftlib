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
#ifndef EB_PLUGIN_TIMING_RECEIVER_HPP_
#define EB_PLUGIN_TIMING_RECEIVER_HPP_

#include <deque>
#include <memory>
#include <string>

#include <saftbus/service.hpp>

#include <sys/stat.h>

#include "eb-source.hpp"

namespace eb_plugin {

class SAFTd;
class TimingReceiver {
public:
	TimingReceiver(saftbus::Container *container, SAFTd *saftd, etherbone::Socket &socket, const std::string &object_path, const std::string &name, const std::string etherbone_path);
	~TimingReceiver();

	// // This function chooses a random address from the msi address range and returns it.
	// // The provided slot will be called whenever the hardware writes to this address.
	// eb_address_t request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot);
	// void release_irq(eb_address_t);

	// static void hook_it_all(etherbone::Socket s);
	// static sigc::connection attach(const std::shared_ptr<Slib::MainLoop>& loop);

	// static void set_msi_buffer_capacity(size_t capacity);

	const std::string &get_object_path() const;

	// @saftbus-export
	void Remove();
	// @saftbus-export
	std::string getEtherbonePath() const;
	// @saftbus-export
	std::string getName() const;


private:

	etherbone::Device eb_device;

	saftbus::Container *container; // need a pointer to container to register new Service objects (ActionSink, Condition, ...)
	SAFTd              *saftd; // need a pointer to SAFTd because ther MSI callbacks can be registered

	std::string object_path;
	std::string name;
	std::string etherbone_path;

	struct stat dev_stat;

	eb_address_t first, last; // Msi address range

	eb_address_t base,  mask;

	//   typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
	//   static irqMap irqs;

	//   struct MSI { eb_address_t address, data; };
	//   typedef boost::circular_buffer<MSI> msiQueue;
	//   static msiQueue msis;

	  bool activate_msi_polling;
	  unsigned polling_interval_ms;
	//   bool poll_msi();
	// eb_address_t msi_first;

	// friend class EbSlaveHandler;
	// friend class MSI_Source;
};

} // namespace 

#endif
