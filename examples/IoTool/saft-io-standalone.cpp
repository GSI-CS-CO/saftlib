#include <etherbone.h>

#include <IoControl.hpp>
#include <io_control_regs.h>
#include <Output.hpp>
#include <Input.hpp>

#include <OpenDevice.hpp>

#include <memory>
#include <iostream>
#include <sstream>

class EB 
{
friend class IoController;
public:
	EB(const std::string &etherbone_path) {
		socket.open();
		device.open(socket, etherbone_path.c_str());
	}	
	~EB() {
		device.close();
		socket.close();
	}
private:
	etherbone::Socket socket;
	etherbone::Device device;
};

class IoController : EB
{
public:
	IoController(const std::string &etherbone_path) 
		: EB(etherbone_path) 
		, io_control(EB::device)
	{
	}
	void list(const std::string &name = std::string()) {
		auto &ios = io_control.get_ios();
		for(auto &io: ios) {
			if (name.size() && name != io.getName()) continue;
			int dir = io.getDirection();
			std::cerr << std::setw(15) <<  io.getName();
			if (dir == IO_CFG_FIELD_DIR_OUTPUT)	std::cout << std::setw(10) << "out";
			if (dir == IO_CFG_FIELD_DIR_INPUT)	std::cout << std::setw(10) << " in";
			if (dir == IO_CFG_FIELD_DIR_INOUT)	std::cout << std::setw(10) << "in/out";
			if (dir == IO_CFG_FIELD_DIR_OUTPUT)	std::cout << std::setw(15) << io.getLogicLevelOut();
			if (dir == IO_CFG_FIELD_DIR_INPUT)	std::cout << std::setw(15) << io.getLogicLevelIn();
			if (dir == IO_CFG_FIELD_DIR_INOUT)	std::cout << std::setw(15) << io.getLogicLevelIn()+"/"+io.getLogicLevelOut();
			if (dir == IO_CFG_FIELD_DIR_OUTPUT)	std::cout << std::setw(10) << io.ReadCombinedOutput();
			if (dir == IO_CFG_FIELD_DIR_INPUT)	std::cout << std::setw(10) << io.ReadInput();
			if (dir == IO_CFG_FIELD_DIR_INOUT)	std::cout << std::setw(10) << io.ReadInput() << "/" << io.ReadCombinedOutput();
			std::cerr << std::endl;
		}
	}
	void set_1(const std::string &name) {
		auto &ios = io_control.get_ios();
		for (auto &io: ios) {
			if (io.getName() == name && (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT)) {
				io.setOutputEnable(true);
				io.WriteOutput(true);
				return;
			}
		}
		std::cerr << name << " cannot be driven high" << std::endl;
	}
	void set_0(const std::string &name) {
		auto &ios = io_control.get_ios();
		for (auto &io: ios) {
			if (io.getName() == name && (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT)) {
				io.setOutputEnable(true);
				io.WriteOutput(false);
				return;
			}
		}
		std::cerr << name << " cannot be driven low" << std::endl;		
	}
	void set_Z(const std::string &name) {
		auto &ios = io_control.get_ios();
		for (auto &io: ios) {
			if (io.getName() == name && (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT)) {
				io.setOutputEnable(false);
				io.WriteOutput(false);
				return;
			}
		}
		std::cerr << name << " cannot be set to high impedance" << std::endl;		
	}
	void set_special(const std::string &name) {
		auto &ios = io_control.get_ios();
		for (auto &io: ios) {
			if (io.getName() == name && (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT)) {
				io.setSpecialPurposeOut(true);
				return;
			}
		}
		std::cerr << name << " has no special purpose" << std::endl;		
	}
	void unset_special(const std::string &name) {
		auto &ios = io_control.get_ios();
		for (auto &io: ios) {
			if (io.getName() == name && (io.getDirection() == IO_CFG_FIELD_DIR_OUTPUT || io.getDirection() == IO_CFG_FIELD_DIR_INOUT)) {
				io.setSpecialPurposeOut(false);
				return;
			}
		}
		std::cerr << name << " has no special purpose" << std::endl;		
	}

private:
	saftlib::IoControl io_control;
};


int main(int argc, char *argv[]) {
	if (argc < 2 || argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
		std::cout << "usage: " << argv[0] << " <eb-device> [ <io-name> [ <level> | <mode> ] ]" << std::endl;
		std::cout << "    eb-device  : etherbone path, e.g. dev/ttyUSB0          " << std::endl;
		std::cout << "    io-name    : name of io, e.g. IO1                      " << std::endl;
		std::cout << "    level      : 1 or 0 or Z, where 1 drives output high,  " << std::endl;
		std::cout << "                                    0 drives output low,   " << std::endl;
		std::cout << "                                    Z doesn't drive output " << std::endl;
		std::cout << "    mode       : special or normal, where special enables  " << std::endl;
		std::cout << "                                    special purpos function" << std::endl;
		std::cout << "                                    and normal disables it " << std::endl;
		std::cout << std::endl;
		std::cout << "    If only <eb-device> is provided, status of all IOs is shown" << std::endl;
		std::cout << "    If <io-name> is provided, status of given IO is shown      " << std::endl;
		std::cout << "    If <level> is provided, given IO is set to this level      " << std::endl;
		std::cout << "    If <mode> is provided, given IO is set to this mode        " << std::endl;
		std::cout << std::endl;
		std::cout << "examples:                                        " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0             " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0 OUT special " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0 IN          " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0 OUT 1       " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0 OUT 0       " << std::endl;
		std::cout << "    ./saft-io-standalone dev/ttyUSB0 OUT Z       " << std::endl;
		return 0;
	}

	std::string eb_path = argv[1];
	IoController controller(eb_path);

	if (argc == 2) {
		controller.list();
		return 0;
	}

	if (argc == 3) {
		controller.list(argv[2]);
		return 0;
	}

	if (argc == 4) {
		if (std::string(argv[3]) == "1") {
			controller.set_1(argv[2]);
			return 0;
		}
		if (std::string(argv[3]) == "0") {
			controller.set_0(argv[2]);
			return 0;
		}
		if (std::string(argv[3]) == "Z") {
			controller.set_Z(argv[2]);
			return 0;
		}
		if (std::string(argv[3]) == "special") {
			controller.set_special(argv[2]);
			return 0;
		}
		if (std::string(argv[3]) == "normal") {
			controller.unset_special(argv[2]);
			return 0;
		}
	}

	return 0;
}
