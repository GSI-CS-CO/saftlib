#include "SAFTd_service.hpp"

#include <loop.hpp>



#include <iostream>
#include <queue>
#include <cstring>
#include <stdexcept>

namespace saftlib 
{



	class EB_Source : public saftbus::Source
	{
		public:
			EB_Source(etherbone::Socket socket_);
		protected:
			
			static int add_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
			static int get_fd(eb_user_data_t, eb_descriptor_t, uint8_t mode);
		
			bool prepare(std::chrono::milliseconds& timeout_ms) override;
			bool check() override;
			bool dispatch() override;
			
		private:
			etherbone::Socket socket;
			
			typedef std::map<int, struct pollfd> fd_map;
			fd_map fds;
	};


	EB_Source::EB_Source(etherbone::Socket socket_)
	 : socket(socket_)
	{}

	int EB_Source::add_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		//std::cerr << "EB_Source::add_fd(" << fd << ")" << std::endl;
		EB_Source* self = (EB_Source*)data;
		
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLERR;

		std::pair<fd_map::iterator, bool> res = 
			self->fds.insert(fd_map::value_type(fd, pfd));
		
		if (res.second) // new element; add to poll
			self->add_poll(res.first->second);
		
		int flags = POLLERR | POLLHUP;
		if ((mode & EB_DESCRIPTOR_IN)  != 0) flags |= POLLIN;
		if ((mode & EB_DESCRIPTOR_OUT) != 0) flags |= POLLOUT;
		
		res.first->second.events |= flags;
		return 0;
	}

	int EB_Source::get_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		EB_Source* self = (EB_Source*)data;
		
		fd_map::iterator i = self->fds.find(fd);
		if (i == self->fds.end()) return 0;
		
		int flags = i->second.revents;
		
		return 
			((mode & EB_DESCRIPTOR_IN)  != 0 && (flags & (POLLIN  | POLLERR | POLLHUP)) != 0) ||
			((mode & EB_DESCRIPTOR_OUT) != 0 && (flags & (POLLOUT | POLLERR | POLLHUP)) != 0);
	}

	static int no_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		return 0;
	}

	bool EB_Source::prepare(std::chrono::milliseconds& timeout_ms)
	{
		std::cerr << "EB_Source::prepare" << std::endl;
		// Retrieve cached current time
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		
		// Work-around for no TX flow control: flush data now
		socket.check(now_sec, 0, &no_fd);
		
		// Clear the old requested FD status
		for (fd_map::iterator i = fds.begin(); i != fds.end(); ++i)
			i->second.events = POLLERR;
		
		// Find descriptors we need to watch
		socket.descriptors(this, &EB_Source::add_fd);
		
		// Eliminate any descriptors no one cares about any more
		fd_map::iterator i = fds.begin();
		while (i != fds.end()) {
			fd_map::iterator j = i;
			++j;
			if ((i->second.events & POLLHUP) == 0) {
				remove_poll(i->second);
				fds.erase(i);
			}
			i = j;
		}
		
		// Determine timeout
		uint32_t timeout = socket.timeout();
		if (timeout) {
			timeout_ms = std::chrono::milliseconds(static_cast<int64_t>(timeout)*1000 - now_sec);
			if (timeout_ms < std::chrono::milliseconds(0)) timeout_ms = std::chrono::milliseconds(0);
		} else {
			timeout_ms = std::chrono::milliseconds(-1);
		}
		
		return (timeout_ms == std::chrono::milliseconds(0)); // true means immediately ready
	}

	bool EB_Source::check()
	{
		std::cerr << "EB_Source::check" << std::endl;
		bool ready = false;
		
		// Descriptors ready?
		for (fd_map::const_iterator i = fds.begin(); i != fds.end(); ++i)
			ready |= (i->second.revents & i->second.events) != 0;
		
		// Timeout ready?
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		ready |= socket.timeout() >= now_sec;
		
		return ready;
	}

	bool EB_Source::dispatch()
	{
		 std::cerr << "EB_Source::dispatch" << std::endl;
		// Process any pending packets
		int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		socket.check(now_sec, this, &EB_Source::get_fd);
		
		// Run the no-op signal
		return true;
	}


//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////





	// Saftlib devices just add IRQs
	class Device : public etherbone::Device {
		public:
			Device(etherbone::Device d, eb_address_t first, eb_address_t last);//, bool poll = false, unsigned piv = 1);
			
			eb_address_t request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot);
			void release_irq(eb_address_t);
			
			static void hook_it_all(etherbone::Socket s);

			// static void set_msi_buffer_capacity(size_t capacity);
			
		private:
			eb_address_t base;
			eb_address_t mask;
			
			typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
			static irqMap irqs;

			struct MSI { eb_address_t address, data; };
			typedef std::queue<MSI> msiQueue;
			static msiQueue msis;
			
			// bool activate_msi_polling;
			// unsigned polling_interval_ms;
			// bool poll_msi();
			eb_address_t msi_first;

		friend class IRQ_Handler;
		friend class MSI_Source;
	};





	Device::irqMap Device::irqs;
	Device::msiQueue Device::msis;

	Device::Device(etherbone::Device d, eb_address_t first, eb_address_t last)//, bool poll, unsigned piv)
	 : etherbone::Device(d), base(first), mask(last-first)//, activate_msi_polling(poll), polling_interval_ms(piv)
	{
	}

	eb_address_t Device::request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot)
	{
		eb_address_t irq;
		
		// Confirm we had SDB records for MSI all the way down
		if (sdb.msi_last < sdb.msi_first) {
			throw etherbone::exception_t("request_irq/non_msi_crossbar_inbetween", EB_FAIL);
		}
		
		// Confirm that first is aligned to size
		eb_address_t size = sdb.msi_last - sdb.msi_first;
		if ((sdb.msi_first & size) != 0) {
			throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
		}
		
		// Confirm that the MSI range could contain our master (not mismapped)
		if (size < mask) {
			throw etherbone::exception_t("request_irq/badly_mapped", EB_FAIL);
		}
		
		// Select an IRQ
		int retry;
		for (retry = 1000; retry > 0; --retry) {
			irq = (rand() & mask) + base;
			irq &= ~7;
			if (irqs.find(irq) == irqs.end()) break;
		}
		
		if (retry == 0) {
			throw etherbone::exception_t("request_irq/no_free", EB_FAIL);
		}

		// if (activate_msi_polling) {
		// 	Slib::signal_timeout().connect(sigc::mem_fun(this, &Device::poll_msi), polling_interval_ms);
		// 	activate_msi_polling = false;
		// }
		
		msi_first = sdb.msi_first;
		// Bind the IRQ
		irqs[irq] = slot;
		// Report the MSI as seen from the point-of-view of the slave
		return irq + sdb.msi_first;
	}

	void Device::release_irq(eb_address_t irq)
	{
		irqs.erase((irq & mask) + base);
	}

	// bool Device::poll_msi() {
	// 	DRIVER_LOG("USB-poll-for-MSIs",-1,-1); 
	// 	etherbone::Cycle cycle;
	// 	eb_data_t msi_adr = 0;
	// 	eb_data_t msi_dat = 0;
	// 	eb_data_t msi_cnt = 0;
	// 	bool found_msi = false;
	// 	const int MAX_MSIS_IN_ONE_GO = 3; // not too many MSIs at once to not block saftd 
	// 	for (int i = 0; i < MAX_MSIS_IN_ONE_GO; ++i) { // never more this many MSIs in one go
	// 		cycle.open(*(etherbone::Device*)this);
	// 		cycle.read_config(0x40, EB_DATA32, &msi_adr);
	// 		cycle.read_config(0x44, EB_DATA32, &msi_dat);
	// 		cycle.read_config(0x48, EB_DATA32, &msi_cnt);
	// 		cycle.close();
	// 		if (msi_cnt & 1) {
	// 			Device::MSI msi;
	// 			msi.address = msi_adr-msi_first;
	// 			DRIVER_LOG("polled-MSI-adr",-1,msi.address); 
	// 			msi.data    = msi_dat;
	// 			// make sure the circular buffer is large enough
	// 			if (Device::msis.size() == Device::msis.capacity()) {
	// 				std::cerr << "Device: change msi fifo capacity from " << std::dec << Device::msis.size() << " to " << 2*Device::msis.size() << std::endl;
	// 				Device::msis.set_capacity(Device::msis.capacity()*2);
	// 			}
	// 			Device::msis.push(msi);
	// 			if (saftbus::device_msi_max_size < Device::msis.size()) {
	// 				saftbus::device_msi_max_size = Device::msis.size();
	// 			}
	// 			found_msi = true;
	// 			DRIVER_LOG("polled-MSI-dat",-1,msi.data); 
	// 		}
	// 		if (!(msi_cnt & 2)) {
	// 			// no more msi to poll
	// 			break; 
	// 		}
	// 	}
	// 	if ((msi_cnt & 2) || found_msi) {
	// 		// if we polled MAX_MSIS_IN_ONE_GO but there are more MSIs
	// 		// OR if there was at least one MSI present 
	// 		// we have to schedule the next check immediately because the 
	// 		// MSI we just polled may cause actions that trigger other MSIs.
	// 		Slib::signal_timeout().connect(sigc::mem_fun(this, &Device::poll_msi), 0);
	// 	} else {
	// 		// if there was no MSI present we continue with the normal polling schedule
	// 		Slib::signal_timeout().connect(sigc::mem_fun(this, &Device::poll_msi), polling_interval_ms);
	// 	}
	// 	DRIVER_LOG("USB-poll-for-MSIs done",-1,-1); 

	// 	return false;
	// }

	struct IRQ_Handler : public etherbone::Handler
	{
		eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
		eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
	};

	eb_status_t IRQ_Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data)
	{
		*data = 0;
		return EB_OK;
	}

	eb_status_t IRQ_Handler::write(eb_address_t address, eb_width_t width, eb_data_t data)
	{
		Device::MSI msi;
		msi.address = address;
		msi.data = data;
		// make sure the circular buffer is large enough
		// if (Device::msis.size() == Device::msis.capacity()) {
		// 	Device::msis.set_capacity(Device::msis.capacity()*2);
		// }
		Device::msis.push(msi);
		// if (saftbus::device_msi_max_size < Device::msis.size()) {
		// 	saftbus::device_msi_max_size = Device::msis.size();
		// }
		return EB_OK;
	}

	void Device::hook_it_all(etherbone::Socket socket)
	{
		static sdb_device everything;
		static IRQ_Handler handler;
		
		everything.abi_class     = 0;
		everything.abi_ver_major = 0;
		everything.abi_ver_minor = 0;
		everything.bus_specific  = SDB_WISHBONE_WIDTH;
		everything.sdb_component.addr_first = 0;
		everything.sdb_component.addr_last  = UINT32_C(0xffffffff);
		everything.sdb_component.product.vendor_id = 0x651;
		everything.sdb_component.product.device_id = 0xefaa70;
		everything.sdb_component.product.version   = 1;
		everything.sdb_component.product.date      = 0x20150225;
		memcpy(everything.sdb_component.product.name, "SAFTLIB           ", 19);
		
		socket.attach(&everything, &handler);
	}

	// void Device::set_msi_buffer_capacity(size_t capacity)
	// {
	// 	msis.set_capacity(capacity);
	// }

	class MSI_Source : public saftbus::Source
	{
		public:
			MSI_Source();
			// static std::shared_ptr<MSI_Source> create();
			// sigc::connection connect(const sigc::slot<bool>& slot);
		
		protected:

			bool prepare(std::chrono::milliseconds& timeout_ms) override;
			bool check() override;
			bool dispatch() override;

		private:
	};


	// std::shared_ptr<MSI_Source> MSI_Source::create()
	// {
	// 	return std::shared_ptr<MSI_Source>(new MSI_Source());
	// }

	// sigc::connection MSI_Source::connect(const sigc::slot<bool>& slot)
	// {
	// 	return connect_generic(slot);
	// }

	MSI_Source::MSI_Source()
	{
	}

	bool MSI_Source::prepare(std::chrono::milliseconds& timeout_ms)
	{
		// returning true means immediately ready
		bool result;
		if (Device::msis.empty()) {
			result = false;
		} else {
			timeout_ms = std::chrono::milliseconds(0);
			result = true;
		}
		return result;
	}

	bool MSI_Source::check()
	{
		bool result = !Device::msis.empty(); // true means ready after glib's poll
		return result;
	}

	bool MSI_Source::dispatch()
	{
		// Don't process more than 10 MSIs in one go (must give dbus some service too)
		int limit = 10;
		
		// Process any pending MSIs
		while (!Device::msis.empty() && --limit) {
			Device::MSI msi = Device::msis.front();
			Device::msis.pop();
			
			Device::irqMap::iterator i = Device::irqs.find(msi.address);
			if (i != Device::irqs.end()) {
				try {
					i->second(msi.data);
				} catch (const etherbone::exception_t& ex) {
					std::cerr << "Unhandled etherbone exception in MSI handler for 0x" 
							 << std::hex << msi.address << ": " << ex << std::endl;
				} catch (std::exception& ex) {
					std::cerr << "Unhandled std::exception exception in MSI handler for 0x" 
							 << std::hex << msi.address << ": " << ex.what() << std::endl;
				} catch (...) {
					std::cerr << "Unhandled unknown exception in MSI handler for 0x" 
							 << std::hex << msi.address << std::endl;
				}
			} else {
				std::cerr << "No handler for MSI 0x" << std::hex << msi.address << std::endl;
			}
		}
		
		return true;
	}





//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////

	struct OpenDevice {
	  Device device;
	  std::string name;
	  std::string objectPath;
	  std::string etherbonePath;
	  // std::shared_ptr<BaseObject> ref;
	  
	  OpenDevice(etherbone::Device d, eb_address_t first, eb_address_t last,
	  	const std::string &n, const std::string &op, const std::string ebp)
	  : device(d, first, last), name(n), objectPath(op), etherbonePath(ebp) 
	  {}
	};



	// bool timeout() {
	// 	static int i = 0;
	// 	std::cerr << "timeout #" << i++ << std::endl;
	// 	// auto core_service_proxy = saftbus::ContainerService_Proxy::create();
	// 	// saftbus::Loop::get_default().quit();
	// 	if (i == 5) {
	// 		return false;
	// 	}
	// 	return true;
	// }

	SAFTd_Service::SAFTd_Service()
		: saftbus::Service(gen_interface_names())
	{
		// std::cerr << "connect timeout source" << std::endl;
		// saftbus::Loop::get_default().connect(
		// 	std::move(
		// 		std2::make_unique<saftbus::TimeoutSource>(
		// 			sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
		// 		) 
		// 	)
		// );

		std::cerr << "SAFTd_Service constructor socket.open()" << std::endl;
		socket.open();
		std::cerr << "SAFTd_Service constructor attach eb_source" << std::endl;
		auto eb_source_unique = std2::make_unique<EB_Source>(socket);
		eb_source = eb_source_unique.get();
		saftbus::Loop::get_default().connect(std::move(eb_source_unique));
		std::cerr << "SAFTd_Service constructor Device::hook_it_all" << std::endl;
		Device::hook_it_all(socket);
		auto msi_source_unique = std2::make_unique<MSI_Source>();
		msi_source = msi_source_unique.get();
		saftbus::Loop::get_default().connect(std::move(msi_source_unique));

		// open the device
		etherbone::Device edev;
		edev.open(socket, "tcp/scuxl0089.acc");
		eb_address_t first, last;
		edev.enable_msi(&first, &last);
		// std::cerr << "first=" << std::hex << first << " last=" << last << std::dec << std::endl;
		// eb_data_t dat;
		// edev.read(0x20140000,EB_DATA32,&dat);
		// std::cerr << "test read at adr 0x20140000 = " << std::hex << dat << std::dec << std::endl;
		device = std::move(std2::make_unique<Device>(edev, first, last));
		// device->read(0x20140000,EB_DATA32,&dat);
		// std::cerr << "test read 2 at adr 0x20140000 = " << std::hex << dat << std::dec << std::endl;


		auto MSI_MAILBOX_VENDOR  = 0x651;
		auto MSI_MAILBOX_PRODUCT = 0xfab0bdd8;
		std::vector<etherbone::sdb_msi_device> mailbox_msi;
		device->sdb_find_by_identity_msi(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mailbox_msi);
		auto irq_adr = device->request_irq(mailbox_msi[0], sigc::mem_fun(this, &SAFTd_Service::msi_handler));
		std::cerr << "irq adr = " << std::hex << irq_adr << std::dec << std::endl;

		std::cerr << "SAFTd_Service constructor done" << std::endl;
	}
	void SAFTd_Service::msi_handler(eb_data_t msi)
	{
		std::cerr << "msi_handler " << msi << std::endl;
	}

	SAFTd_Service::~SAFTd_Service() {
		for(auto &dev: devs) {
			dev.second->device.close();
		}
		std::cerr << "SAFTd_Service destructor" << std::endl;
		saftbus::Loop::get_default().remove(msi_source);
		device->close();
		saftbus::Loop::get_default().remove(eb_source);
		socket.close();

		std::cerr << "SAFTd_Service destructor done" << std::endl;
	}

	std::vector<std::string> SAFTd_Service::gen_interface_names()
	{
		std::vector<std::string> result;
		result.push_back("de.gsi.saftlib");
		return result;
	}

	void SAFTd_Service::call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send)
	{
		switch(interface_no) {
			case 0:
				switch(function_no) {
					case 0:  { // eb-read
						try {
							eb_address_t adr;
							eb_data_t dat;
							std::cerr << "eb-write called" << std::endl;
							received.get(adr);
							std::cerr << "test read interactive at adr " << std::hex << adr << "   =>  " << dat << std::dec << std::endl;
							device->read(adr,EB_DATA32,&dat);
							send.put(dat);
						} catch (etherbone::exception_t &e) {
							std::cerr << "got etherbone exception_t " << e.method << std::endl;
							send.put(0xdeadbeef);
						}
					}
					break;
					case 1:  { // AttachDevice
						std::string name, path;
						std::cerr << "AttachDevice called" << std::endl;
						received.get(name);
						received.get(path);
						send.put(AttachDevice(name,path));
					}
					break;
					case 2:  { // eb-write
						std::string name;
						received.get(name);
						RemoveDevice(name);
					}
					break;
					default:
						std::cerr << "unknown function_no " << function_no << std::endl;
				}
			break;
			default:
				std::cerr << "unknown interface_no " << interface_no << " " << std::endl;
		}
	}



	static inline bool not_isalnum_(char c) 
	{ 
		return !(isalnum(c) || c == '_');
	}

	std::string SAFTd_Service::AttachDevice(const std::string& name, const std::string& path)
	{
		if (devs.find(name) != devs.end())
			throw std::runtime_error("device already exists");
		if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
			throw std::runtime_error("Invalid name; [a-zA-Z0-9_] only");
		
		try {
			etherbone::Device edev;
			edev.open(socket, path.c_str());
			
			try {
				// Determine the size of the MSI range
				eb_address_t first, last;
				edev.enable_msi(&first, &last);
			
				// Confirm the device is an aligned power of 2
				eb_address_t size = last - first;
			
				if (((size + 1) & size) != 0)
					throw std::runtime_error("Device has strange sized MSI range");
			
				if ((first & size) != 0)
					throw std::runtime_error("Device has unaligned MSI first address");
			
				// bool poll_msis = false;
				// std::string path_prefix = path.substr(0,7);
				// // enable polling MSIs for USB and pseudo-terminal devices
				// if (path_prefix == "dev/tty" || path_prefix == "dev/pts") {
				// 	poll_msis = true;
				// }
				// struct OpenDevice od(edev, first, last, poll_msis);
				// od.name = name;
				// od.objectPath = "/de/gsi/saftlib/" + name;
				// od.etherbonePath = path;


			
				//Drivers::probe(od);
				// if (od.ref) {
					devs.insert(std::make_pair(name, std2::make_unique<OpenDevice>(edev, first, last, name, get_object_path()+name, path)));

					// if (poll_msis) {
					// 	// create a special socket for eb-tools to attach to.
					// 	m_eb_forward[name] = std::shared_ptr<EB_Forward>(new EB_Forward(path));
					// }

					return get_object_path() + "/"+  name;
				// } else {
				// 	throw std::runtime_error("no driver available for this device");
				// }
			} catch (...) {
				edev.close();
				throw;
			}
		} catch (const etherbone::exception_t& e) {
			std::ostringstream str;
			str << "AttachDevice: failed to open: " << e;
			throw std::runtime_error(str.str().c_str());
		}
	}

	void SAFTd_Service::RemoveDevice(const std::string& name)
	{
		std::map< std::string, std::unique_ptr<OpenDevice> >::iterator elem = devs.find(name);
		if (elem == devs.end())
			throw std::runtime_error("no such device");

		// remove special socket for eb-tools  
		// m_eb_forward.erase(name);
		///////////////////////////

		// elem->second.ref.reset();
		elem->second->device.close();
		devs.erase(elem);
	}













}

extern "C" std::unique_ptr<saftbus::Service> create_service() {
	return std2::make_unique<saftlib::SAFTd_Service>();
}
