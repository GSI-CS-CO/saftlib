/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author M. Reese <m.reese@gsi.de>
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

// Software emulation of a TimingReceiver ECA
//
// The first part of this program consists of an Etherbone slave implementation that was 
// originally developed to connect GHDL simulations to real host programs. It behaves like 
// a usb serial device and is implemented as a pseudo-terminal in /dev/pts/<n>, where <n> 
// is an integer thas is determined by the OS. After startup, the full devicename ist written
// into the temporary file "/tmp/simbridge-eb-device", such that eb-tools can be used like
// this: "eb-ls $(cat /tmp/simbridge-eb-device)"
//
// The second part of this program consists of an emulation of some hardware registers 
// in a TimingReceiver and partially of its behavior. Register layout is based on a SDB record file 
// from a real TimingReceiver. This Program can also be used to extract such an SDB record file
// from existing hardware (use the --extract-sdb <devicename> option and redirect the 
// output into a file).
// Currently, the program implements enough TimingReceiver functionality that saftlib can connect 
// to it (using the pseudo-terminal device).
// Saftlib clients can create software action sinks and inject events into the simulated hardware.
// The timing events are redistributed to the correct SoftwarActionSinks just as they would be 
// if saftlib would work on real hardware. The functionality is limited in that flags such as 
// LATE,CONFLICT,DELAYED,EARLY are not supported. Late events are alywas delivered immediately. 
// The execution time comes from the system time and is not synchronized to a WhiteRabbit network.
//
// Besides parts of the ECA, not much of the hardware behavior is currently implemented. 

// global variable that controls the amout of output created by all parts of the program
// verbosity = -1: output only error messages
// verbosity =  0: output program status information
// verbosity =  1: output register access
int verbosity = 0;

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <deque>

// std_logic values
typedef enum { 
	STD_LOGIC_U,
	STD_LOGIC_X,
	STD_LOGIC_0,
	STD_LOGIC_1,
	STD_LOGIC_Z,
	STD_LOGIC_W,
	STD_LOGIC_L,
	STD_LOGIC_H,
	STD_LOGIC_DASH
} std_logic_t;


class EBslave 
{
public:
	void init();
	void shutdown();

	EBslave(bool stop_until_connected, uint32_t sdb_adr, uint32_t msi_addr_first, uint32_t msi_addr_last); 
	~EBslave() {
		std::cerr << "closing fd" << std::endl;
		close(pfds[0].fd);
		std::cerr << "destructor done" << std::endl;
	}
	std::string pts_name();


	void fill_input_buffer();
	// return true if a word is available
	// return false if there is nothing
	bool next_word(uint32_t &result);


	int master_out(std_logic_t *cyc, std_logic_t *stb, std_logic_t *we, int *adr, int *dat, int *sel);

	int handle_pass_through();

	void send_output_buffer();

	// should be called on falling_edge(clk)
	int master_in(std_logic_t ack, std_logic_t err, std_logic_t rty, std_logic_t stall, int dat);



	void msi_slave_out(std_logic_t *ack, std_logic_t *err, std_logic_t *rty, std_logic_t *stall, int *dat);

	void msi_slave_in(std_logic_t cyc, std_logic_t stb, std_logic_t we, int adr, int dat, int sel);

	void push_msi(uint32_t adr, uint32_t dat);

private:
	struct pollfd pfds[1];	
	std::deque<uint32_t> input_word_buffer;
	std::deque<uint32_t> input_word_buffer2; // only used to echo the input next to the output (not used for bridge logic)
	std::deque<uint32_t> output_word_buffer;
	bool eb_flag_bca = false;
	bool eb_flag_rca = false;
	bool eb_flag_rff = false;
	bool eb_flag_cyc = false;
	bool eb_flag_wca = false;
	bool eb_flag_wff = false;
	uint8_t eb_byte_en, eb_wcount, eb_rcount;

	uint32_t base_write_adr;
	uint32_t base_ret_adr;


	uint32_t error_shift_reg;
	uint32_t eb_sdb_adr;
	uint32_t eb_msi_adr_first;
	uint32_t eb_msi_adr_last;

	bool msi_slave_out_ack;
	bool msi_slave_out_err;

	struct MSI {
		MSI(uint32_t a, uint32_t d) : adr(a), dat(d) {}
		uint32_t adr;
		uint32_t dat;
	};
	std::deque<MSI> msi_queue;
	uint32_t msi_adr;
	uint32_t msi_dat;
	uint32_t msi_cnt;


	// state machine of the EB-slave
	typedef enum{
		EB_SLAVE_STATE_IDLE,
		EB_SLAVE_STATE_EB_HEADER,
		EB_SLAVE_STATE_EB_CONFIG_FIRST,
		EB_SLAVE_STATE_EB_CONFIG_REST,
		EB_SLAVE_STATE_EB_WISHBONE_FIRST,
		EB_SLAVE_STATE_EB_WISHBONE_REST,
	} state_t;
	state_t state;

	uint32_t word_count;

	bool strobe;

	uint32_t new_header;

	struct wb_stb {
		uint32_t adr;
		uint32_t dat;
		bool we;
		bool ack;
		bool err;
		bool passthrough;
		bool zero;
		bool end_cyc;
		bool new_header;
		uint32_t new_header_value;
		std::string comment;
		wb_stb(uint32_t a, uint32_t d, bool w, bool pt = false) 
		: adr(a), dat(d), we(w), ack(false), err(false), passthrough(pt), zero(false), end_cyc(false), new_header(false) {};
	};
	std::deque<wb_stb> wb_stbs;
	std::deque<wb_stb> wb_wait_for_acks;

	bool _stop_until_connected;
	bool _shutdown;
};

void EBslave::shutdown() {
	_shutdown = true;
}

void EBslave::init() {
	wb_stbs.clear();
	wb_wait_for_acks.clear();
	input_word_buffer.clear();
	input_word_buffer2.clear();
	output_word_buffer.clear();

	if (_shutdown) return; // dont open the device if shutdown was initiated;

	pfds[0].fd = open("/dev/ptmx", O_RDWR );//| O_NONBLOCK);

	// put it in raw mode
	struct termios raw;
	if (tcgetattr(pfds[0].fd, &raw) == 0)
	{
		// input modes - clear indicated ones giving: no break, no CR to NL, 
		//   no parity check, no strip char, no start/stop output (sic) control 
		raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

		// output modes - clear giving: no post processing such as NL to CR+NL 
		raw.c_oflag &= ~(OPOST);

		// control modes - set 8 bit chars 
		raw.c_cflag |= (CS8);

		// local modes - clear giving: echoing off, canonical off (no erase with 
		//   backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) 
		raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

		// control chars - set return condition: min number of bytes and timer 
		raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 8; // after 5 bytes or .8 seconds
		//                                          //   after first byte seen   
		raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; // immediate - anything      
		raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0; // after two bytes, no timer 
		raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; // after a byte or .8 seconds

		// put terminal in raw mode after flushing 
		if (tcsetattr(pfds[0].fd,TCSAFLUSH,&raw) < 0) 
		{
			int err = errno;
			std::cerr << "Error, cant set raw mode: " << strerror(err) << std::endl;
			return;
		}
	}
	word_count = 0;
	grantpt(pfds[0].fd);
	unlockpt(pfds[0].fd);
	state = EB_SLAVE_STATE_IDLE;
	std::ofstream tmpfile("/tmp/simbridge-eb-device");
	tmpfile << pts_name().substr(1) << std::endl;
	if (verbosity >= 0) {
		std::cout << "eb-device: " << pts_name() << std::endl;
		if (_stop_until_connected) {
			std::cout << "waiting for client, simulation stopped ... ";
		} else {
			std::cout << "device is ready, simulation is running" << std::endl;
		}
	}
	if (_stop_until_connected)
	{
		pfds[0].events = POLLIN;
		poll(pfds,1,-1);
		if (verbosity >= 0) {
			std::cout << " connected, simulation continues" << std::endl;
		}
	}
	error_shift_reg = 0;

}


EBslave::EBslave(bool stop_until_connected, uint32_t sdb_adr, uint32_t msi_addr_first, uint32_t msi_addr_last) 
{
	_stop_until_connected = stop_until_connected;
	_shutdown = false;
	eb_sdb_adr       = sdb_adr;
	eb_msi_adr_first = msi_addr_first;
	eb_msi_adr_last  = msi_addr_last;
	init();
}

std::string EBslave::pts_name() {
	return std::string(ptsname(pfds[0].fd));
}


void EBslave::fill_input_buffer() {
	uint8_t buffer[1024];
	uint8_t *value;
	pfds[0].events = POLLIN;
	int timeout_ms = 1;
	int result = poll(pfds,1,timeout_ms);
	if (result != 1) {
		return;
	}
	if (pfds[0].revents == POLLHUP) {
		close(pfds[0].fd);
		pfds[0].fd = 0;
		// exit(1);
		init();
		return;
	}
	result = read(pfds[0].fd, (void*)buffer, sizeof(buffer));
	if (result == -1 && errno == EAGAIN) {
		return;
	} else if (result == -1) {
		std::cerr << "unexpected error " << errno << " " << strerror(errno) << std::endl;
		switch(errno) {
			case EBADF:  std::cerr << "EBADF" << std::endl; break;
			case EFAULT: std::cerr << "EFAULT" << std::endl; break;
			case EINTR:  std::cerr << "EINTR" << std::endl; break;
			case EINVAL: std::cerr << "EINVAL" << std::endl; break;
			case EIO:    std::cerr << "EIO" << std::endl; break;
			case EISDIR: std::cerr << "EISDIR" << std::endl; break;
		}
		close(pfds[0].fd);
		pfds[0].fd = 0;
		init();

	} else if (result >= 4) {
		value = buffer;
		while (result > 0) {
			uint32_t value32  = value[0]; value32 <<=8;
			         value32 |= value[1]; value32 <<=8;
			         value32 |= value[2]; value32 <<=8;
			         value32 |= value[3]; 
			// std::cerr << "<= 0x" << std::hex << std::setw(8) << std::setfill('0') << (uint32_t)value32 << std::endl;
			input_word_buffer.push_back(value32);
			input_word_buffer2.push_back(value32);
			result -= 4;
			value  += 4; 
			++word_count;
		}
	}
}
// return true if a word is available
// return false if there is nothing
bool EBslave::next_word(uint32_t &result) {
	// try to read from input stream
	for (;;) {
		if (input_word_buffer.size() > 0) {
			result = input_word_buffer.front();
			input_word_buffer.pop_front();
			return true;
		} else {
			fill_input_buffer();
			if (input_word_buffer.size() > 0) continue;
			return false;
		}
	}
}


int EBslave::master_out(std_logic_t *cyc, std_logic_t *stb, std_logic_t *we, int *adr, int *dat, int *sel) {
	int end_cyc = 0;

	*cyc = STD_LOGIC_0;
	*stb = STD_LOGIC_0;
	*we  = STD_LOGIC_0;
	*adr = 0xaffe1234;
	*dat = 0xbabe1234;
	*sel = 0xf;

	uint32_t word;
	switch(state) {
		case EB_SLAVE_STATE_IDLE:
			if (next_word(word)) {
				if (word == 0x4e6f11ff) {
					wb_stbs.push_back(wb_stb(0,0x4e6f1644,false,true)); // not a real strobe, just a pass-through
					if (next_word(word)) {
						wb_stbs.push_back(wb_stb(0,word,false,true)); // not a real strobe, just a pass-through
						state = EB_SLAVE_STATE_EB_HEADER;
					}
				}
			}
		break;
		case EB_SLAVE_STATE_EB_HEADER:
			if (next_word(word)) { // eb record header
				eb_flag_bca =  word & 0x80000000;
				eb_flag_rca =  word & 0x40000000;
				eb_flag_rff =  word & 0x20000000;
				eb_flag_cyc =  word & 0x08000000;
				eb_flag_wca =  word & 0x04000000;
				eb_flag_wff =  word & 0x02000000;
				eb_byte_en  = (word & 0x00ff0000) >> 16;
				eb_wcount   = (word & 0x0000ff00) >>  8;
				eb_rcount   = (word & 0x000000ff) >>  0;
				uint32_t response  = (word & 0x00ff0000); // echo byte_enable
				         //response |= (word & 0x0000ff00) >> 8; // wcount becomes rcount (no! wcount becomes zero)
				         response |= (word & 0x000000ff) << 8; // rcount becomes wcount
				         response |= (eb_flag_cyc << 27); // response rca <= request bca
				         response |= (eb_flag_bca << 26); // response rff <= request rff
				         response |= (eb_flag_rff << 25); // response wca <= request wca;


				// if we have a write request, the response must be zero and a new header has to be inserted 
				// in front of the read response (if there was any read request)
				if (eb_wcount > 0) {
					new_header = response & 0xffffff00; // delete the read count;
					response = 0;
				}
				wb_stbs.push_back(wb_stb(response,response,false,true)); // not a real strobe, just a pass-through
				wb_stbs.back().comment = "header";
				wb_stbs.back().end_cyc = eb_flag_cyc;

				if (eb_wcount > 0) {
				 	if (eb_flag_wca) {
						state = EB_SLAVE_STATE_EB_CONFIG_FIRST;						
					} else {
						state = EB_SLAVE_STATE_EB_WISHBONE_FIRST;
					}
				} else {
					if (eb_flag_rca) { // access to config space
						state = EB_SLAVE_STATE_EB_CONFIG_FIRST;
					} else {
						state = EB_SLAVE_STATE_EB_WISHBONE_FIRST;
					}
				}
			}
		break;
		case EB_SLAVE_STATE_EB_CONFIG_FIRST:
			if (eb_wcount > 0) {
				uint32_t base_write_adr;
				if (next_word(base_write_adr)) {
					wb_stbs.push_back(wb_stb(0x0,0x0,false,true)); // not a real strobe, just a pass-through
					wb_stbs.back().end_cyc = eb_flag_cyc;
					state = EB_SLAVE_STATE_EB_CONFIG_REST;
				}
			} else if (eb_rcount > 0) {
				uint32_t base_ret_adr;
				if (next_word(base_ret_adr)) {
					wb_stbs.push_back(wb_stb(base_ret_adr,base_ret_adr,false,true)); // not a real strobe, just a pass-through
					wb_stbs.back().end_cyc = eb_flag_cyc;
					state = EB_SLAVE_STATE_EB_CONFIG_REST;
				}
			} else {
				state = EB_SLAVE_STATE_EB_HEADER;
			}
		break;
		case EB_SLAVE_STATE_EB_CONFIG_REST:
			// eb slave config space registers
			// x"00000000"                                      when "01000", -- 0x20 = 0[010 00]00
			// x"00000000"                                      when "01001", -- 0x24 = 0[010 01]00
			// x"00000000"                                      when "01010", -- 0x28
			// x"00000001"                                      when "01011", -- 0x2c
			// x"00000000"                                      when "01100", -- 0x30
			// c_ebs_msi.sdb_component.addr_first(31 downto  0) when "01101", -- 0x34
			// x"00000000"                                      when "01110", -- 0x38
			// c_ebs_msi.sdb_component.addr_last(31 downto  0)  when "01111", -- 0x3c
			// msi_adr                                          when "10000", -- 0x40 = 0[100 00]00
			// msi_dat                                          when "10001", -- 0x44 = 0[100 01]00
			// msi_cnt                                          when "10010", -- 0x48 = 0[100 10]00
			// x"00000000"                                      when others;
			if (eb_wcount > 0) {
				uint32_t write_val;
				if (next_word(write_val)) {
					--eb_wcount;
					if (eb_wcount == 0) {
						wb_stbs.push_back(wb_stb(new_header,new_header,false,true)); // not a real strobe, just a pass-through
						wb_stbs.back().end_cyc = eb_flag_cyc;
					} else {
						wb_stbs.push_back(wb_stb(0x0,0x0,false,true)); // not a real strobe, just a pass-through
						wb_stbs.back().end_cyc = eb_flag_cyc;
					}
					if (eb_wcount == 0) {
						if (eb_rcount == 0) {
							state = EB_SLAVE_STATE_EB_HEADER;
						} else {
							state = EB_SLAVE_STATE_EB_CONFIG_FIRST; // handle the rcount values
						}
					}
				}
			} else if (eb_rcount > 0) {
				uint32_t read_adr;
				if (next_word(read_adr)) {

					--eb_rcount;
					switch(read_adr) {
						case 0x0: 
							wb_stbs.push_back(wb_stb(error_shift_reg,error_shift_reg,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
							error_shift_reg = 0; // clear the error shift register 
						break;
						case 0xc: 
						    // this should return the sdb address
							wb_stbs.push_back(wb_stb(eb_sdb_adr,eb_sdb_adr,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x2c: 
							wb_stbs.push_back(wb_stb(0x1,0x1,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x34:
							wb_stbs.push_back(wb_stb(eb_msi_adr_first,eb_msi_adr_first,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x3c:
							wb_stbs.push_back(wb_stb(eb_msi_adr_last,eb_msi_adr_last,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x40: // msi_adr
							if (msi_queue.size() > 0) {
								msi_adr = msi_queue.front().adr;
								msi_dat = msi_queue.front().dat;
								msi_cnt = 1;
								if (msi_queue.size() > 1) {
									msi_cnt = 3;
								}
								msi_queue.pop_front();
							} else {
								msi_cnt = 0;
							}
							wb_stbs.push_back(wb_stb(msi_adr,msi_adr,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x44: // msi_dat
							wb_stbs.push_back(wb_stb(msi_dat,msi_dat,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						case 0x48:
							wb_stbs.push_back(wb_stb(msi_cnt,msi_cnt,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
						break;
						default: 
							wb_stbs.push_back(wb_stb(0x0,0x0,false,true)); // not a real strobe, just a pass-through
							wb_stbs.back().end_cyc = eb_flag_cyc;
							wb_stbs.back().err = true;
					}
					if (eb_rcount == 0) {
						state = EB_SLAVE_STATE_EB_HEADER;
					}
				}

			}
		break;
		case EB_SLAVE_STATE_EB_WISHBONE_FIRST:
			if (eb_wcount > 0) {
				if (next_word(base_write_adr)) {
					// output_word_buffer.push_back(base_write_adr);
					wb_stbs.push_back(wb_stb(0x0,0x0,false,true)); // not a real strobe, just a pass-through
					wb_stbs.back().end_cyc = eb_flag_cyc;
					// wb_stbs.back().zero = true;
					state = EB_SLAVE_STATE_EB_WISHBONE_REST;
				}
			} else if (eb_rcount > 0) {
				if (next_word(base_ret_adr)) {
					// output_word_buffer.push_back(base_ret_adr);
					wb_stbs.push_back(wb_stb(base_ret_adr,base_ret_adr,false,true)); // not a real strobe, just a pass-through
					wb_stbs.back().end_cyc = eb_flag_cyc;
					state = EB_SLAVE_STATE_EB_WISHBONE_REST;
				}
			} else {
				state = EB_SLAVE_STATE_EB_HEADER;
			}
		break;
		case EB_SLAVE_STATE_EB_WISHBONE_REST:
			if (eb_wcount > 0) {
				uint32_t write_val;
				if (next_word(write_val)) {
					--eb_wcount;
					// put the write strobe into the queue
					wb_stbs.push_back(wb_stb(base_write_adr,write_val,true));
					wb_stbs.back().end_cyc = eb_flag_cyc;
					if (eb_wcount == 0) {
						wb_stbs.back().new_header = true;
						wb_stbs.back().new_header_value = new_header;
					} else {
						wb_stbs.back().zero = true;
					}
					// increment base_write_adr unless we are writing into a fifo
					if (!eb_flag_wff) base_write_adr += 4; 
					if (eb_wcount == 0) {
						if (eb_rcount == 0) {
							state = EB_SLAVE_STATE_EB_HEADER;
						} else {
							if (eb_flag_rca) { // access to config space
								state = EB_SLAVE_STATE_EB_CONFIG_FIRST;
							} else {
								state = EB_SLAVE_STATE_EB_WISHBONE_FIRST;
							}	
						}
					}
				}
			} else if (eb_rcount > 0) {
				uint32_t read_adr;
				if (next_word(read_adr)) {
					// std::cerr << "read_adr " << std::hex << std::setw(8) << std::setfill('0') << read_adr 
					//           << "    rcnt " << std::dec << (int)eb_rcount << std::endl;
					--eb_rcount;
					wb_stbs.push_back(wb_stb(read_adr,0,false));
					wb_stbs.back().end_cyc = eb_flag_cyc;
					if (eb_rcount == 0) {
						state = EB_SLAVE_STATE_EB_HEADER;
					}
				}

			}
		break;
	}


	if (handle_pass_through()) end_cyc = 1;
	send_output_buffer();

	if (eb_flag_cyc) {
		*cyc = STD_LOGIC_0;
	}
	if (wb_stbs.size() > 0 || wb_wait_for_acks.size() > 0) {
		*cyc = STD_LOGIC_1;
	}

	strobe = false;
	bool write_enable = false;
	*we = STD_LOGIC_0;
	if (wb_stbs.size() > 0) {
		strobe = true;
		if (wb_stbs.front().end_cyc) {
			end_cyc = 1;
		}
		if (wb_stbs.front().we) {
			*we = STD_LOGIC_1;
			write_enable = true;
		} else {
			*we = STD_LOGIC_0;
		}
		*adr = wb_stbs.front().adr;
		*dat = wb_stbs.front().dat;
		*sel = 0xf;
	}
	*stb = strobe ? STD_LOGIC_1 : STD_LOGIC_0;
	*we  = write_enable ? STD_LOGIC_1 : STD_LOGIC_0;
	return end_cyc;
}

int EBslave::handle_pass_through() {
	int end_cyc = 0;
	// std::cerr << "handle_pass_through " << wb_stbs.size() << std::endl;
	while(wb_stbs.size() > 0 && wb_stbs.front().passthrough) {
		wb_wait_for_acks.push_back(wb_stbs.front());
		if (wb_stbs.front().end_cyc) end_cyc = 1;
		wb_stbs.pop_front();
	}
	// std::cerr << "handle_pass_through " << wb_wait_for_acks.size() << std::endl;
	// remove all pass-through values
	while (wb_wait_for_acks.size() > 0 && wb_wait_for_acks.front().passthrough) {
		// std::cerr << "pass-through" << std::endl;
		output_word_buffer.push_back(wb_wait_for_acks.front().dat);
		int err = wb_wait_for_acks.front().err;
		error_shift_reg = (error_shift_reg << 1) | err;
		wb_wait_for_acks.pop_front();
	}
	// std::cerr << "handle_pass_through " << output_word_buffer.size() << std::endl;
	return end_cyc;
}

void EBslave::send_output_buffer()
{
	if (wb_wait_for_acks.size() == 0 && output_word_buffer.size() >= word_count) {
		std::vector<uint8_t> write_buffer;
		while (output_word_buffer.size() > 0) {
			--word_count;
			uint32_t word_out = output_word_buffer.front();
			// uint32_t word_in  = input_word_buffer2.front();
			output_word_buffer.pop_front();
			input_word_buffer2.pop_front();
			for (int i = 0; i < 4; ++i) {
				uint8_t val = word_out >> (8*(3-i));
				write_buffer.push_back(val);
			}
		}
		if (write_buffer.size() ) {
			write(pfds[0].fd, (void*)&write_buffer[0], write_buffer.size());
		}
	}
}

// should be called on falling_edge(clk)
int EBslave::master_in(std_logic_t ack, std_logic_t err, std_logic_t rty, std_logic_t stall, int dat) {
	int end_cyc = 0;
	if (handle_pass_through()) end_cyc = 1;
	if (wb_stbs.size() > 0 && (strobe && stall == STD_LOGIC_0)) {
		wb_wait_for_acks.push_back(wb_stbs.front());
		if (wb_stbs.front().end_cyc) end_cyc = 1;
		wb_stbs.pop_front();
	}
	if (wb_wait_for_acks.size() > 0 && (ack == STD_LOGIC_1 || err == STD_LOGIC_1)) {
		if (wb_wait_for_acks.front().we) {
			if (wb_wait_for_acks.front().zero) {
				output_word_buffer.push_back(0x0);
			} else if (wb_wait_for_acks.front().new_header) {
				output_word_buffer.push_back(wb_wait_for_acks.front().new_header_value);
			} else {
				output_word_buffer.push_back(wb_wait_for_acks.front().dat);
			}
		} else {
			output_word_buffer.push_back(dat);
		}
		wb_wait_for_acks.pop_front();
		int err = 0;
		if (err == STD_LOGIC_1) {
			err = 1;
		}
		error_shift_reg = (error_shift_reg << 1) | err;
	}		
	send_output_buffer();
	return end_cyc;
}



void EBslave::msi_slave_out(std_logic_t *ack, std_logic_t *err, std_logic_t *rty, std_logic_t *stall, int *dat) {
	*ack = STD_LOGIC_0;
	*err = STD_LOGIC_0;
	*rty = STD_LOGIC_0;
	*stall = STD_LOGIC_0;
	if (msi_slave_out_ack) *ack = STD_LOGIC_1;
	if (msi_slave_out_err) *err = STD_LOGIC_1;
	*dat = 0x0;
}

void EBslave::push_msi(uint32_t adr, uint32_t dat) {
	msi_queue.push_back(MSI(adr,dat));
}

void EBslave::msi_slave_in(std_logic_t cyc, std_logic_t stb, std_logic_t we, int adr, int dat, int sel) {
	msi_slave_out_ack = false;
	msi_slave_out_err = false;
	if (cyc == STD_LOGIC_1 && stb == STD_LOGIC_1) {
		if (we == STD_LOGIC_1) {
			msi_slave_out_ack = true;
			msi_queue.push_back(MSI(adr,dat));
			// ignore sel
		} else {
			msi_slave_out_err = true; // msi_slave is write-only!
		}
	} 
}



#include <cstdint>

#include <time.h>

#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <thread>
#include <mutex>

EBslave *eb_slave = nullptr;
uint32_t eca_msi_target_adr = 0;
uint32_t eca_in_buffer[8] = {0,};
uint32_t eca_tag = 0;



// all classes that mimic hardware behavior are need to be subclasses of this Device class
class Device
{
public:
	virtual bool contains(uint32_t adr) = 0;
	virtual bool read_access(uint32_t adr, int sel, uint32_t *dat_out) { return false; }
	virtual bool write_access(uint32_t adr, int sel, uint32_t dat) { return false; }
};

// This class reads an SDB record file and allows to place other Devices (i.e. Classes 
// derived from class Device) into the address space based on their product and vendor ids.
class SDBrecords : public Device {
public:
	SDBrecords(const std::string &filename) {
		std::ifstream in(filename.c_str());
		if (!in) {
			std::ostringstream out;
			out << "cannot open SDB record file: " << filename;
			throw std::runtime_error(out.str());
		}
		for (int i = 0;; ++i) {
			std::string line;
			std::getline(in, line);
			if (!in) break;
			std::istringstream lin(line);
			uint32_t adr, dat, abs_dat;
			std::string arrow;
			// the SDB-record file consists of three columns: 
			// address of the SDB record, Content of the SDB record, Content of the SDB record puls the base address of that record if it is a link to another SDB-record
			lin >> std::hex >> adr >> arrow >> dat >> arrow >> abs_dat;
			if (!lin) break;
			if (blocks.empty() || blocks.back().second+4 != adr) {
				blocks.push_back(std::make_pair(adr,adr));
			} else {
				blocks.back().second = adr;
			}
			memory[adr] = dat;
			memory_absolute[adr] = abs_dat;
			// the very first address is used as sdb start of the toplevel interconnect
			if (i == 0) {
				start = adr;
			}
		}
	}
	uint32_t start_adr() {
		return start;
	}
	bool contains(uint32_t adr) {
		for (auto &block: blocks) {
			if (adr >= block.first && adr <= block.second) {
				return true;
			}
		}
		return false;
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		auto itr = memory.find(adr);
		if (itr != memory.end()) {
			*dat_out = itr->second;
			return true;
		}
		return false;
	}
	template<class Dev>
	std::vector<std::shared_ptr<Dev> > create_devices() {
		if (verbosity >= 0) {
			std::cout << "looking for device_id " << std::hex << Dev::product_id << std::endl;
		}
		std::vector<std::shared_ptr<Dev> > result;
		for (auto &block: blocks) {
			for (uint32_t block_adr = block.first; block_adr < block.second; block_adr += 16*4) {
				if (block_matches_ids(block_adr, Dev::vendor_id, Dev::product_id)) {
					if (verbosity >= 0) {
						std::cout << "found device 0x" << std::hex << Dev::product_id << std::endl;
					}
					result.push_back(std::make_shared<Dev>(block_device_adr_first(block_adr), result.size()));
				}
			}
		}
		return result;
	}
	bool block_matches_ids(uint32_t block_adr, uint32_t vendor_id, uint32_t device_id) {
		if ((memory[block_adr+4*15]&0xff) == 0x1) {
			return (memory[block_adr+4*7] == vendor_id) && (memory[block_adr+4*8] == device_id);
		}
		return false;
	}
	uint32_t block_device_adr_first(uint32_t block_adr) {
		return memory_absolute[block_adr+4*3];
	}
private:
	uint32_t start;
	std::map<uint32_t, uint32_t> memory;
	std::map<uint32_t, uint32_t> memory_absolute; // relative addresses replaced by absolute values
	std::vector<std::pair<uint32_t, uint32_t> > blocks;
};

class WatchdogMutex : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0xb6232cd3,
	};
	WatchdogMutex(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{}
	bool contains(uint32_t adr) {
		return adr == _adr_first; // WatchdogMutex has only one register
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		if (verbosity >= 1) {
			std::cout << "WatchdogMutex read access "  << std::endl;
		}
		*dat_out = 0x0;
		return true; 
	}
	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		if (verbosity >= 1) {
			std::cout << "WatchdogMutex write access "  << std::hex << dat << std::endl;
		}
		return true; 
	}
private:
	uint32_t _adr_first;
	int      _instance;
};



class FpgaReset : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0x3a362063,
	};
	FpgaReset(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 0) {
			std::cout << "FpgaReset " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		if ( (adr >= _adr_first) && (adr < _adr_first + 0x100) ) {
			return true;
		} else {
			return false;
		}
	}
	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		if (dat == 0xdeadbeef) {
			std::cerr << "write access to FpgaReset => quit" << std::endl;
			_reset_was_triggered = true;
		}
		return true;
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {

		return true;
	}

	static bool _reset_was_triggered;
private:
	uint32_t _adr_first;
	int      _instance;
};
bool FpgaReset::_reset_was_triggered = false;


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
// BEGINNING OF ECA RELATED CLASSES
//////////////////////////////////////////////////////////////////
// THE SOFTWARE ECA
//
// The software eca consists of a woker class that does the actual ECA functionality:
//   * contains a list of search entries that is extracted from the data saftlib writes 
//     into the ECA control registers
//   * contains walker entries that are connected to saftlibs SoftwarActionSinks
//   * can inject events and creates the correct MSIs in the EB slave such that saftlib
//     can poll them (similar to the USB bridge).
//
// Multiple classes derived from class Device mimic register access to the ECA sub components
// just like in the real Hardware (saftlib will not know the difference)

#include "../drivers/eca_flags.h"
#include "../drivers/eca_regs.h"
#include "../drivers/eca_queue_regs.h"

// This class does in software (roughly) what the real ECA does in hardware.
// Limitations: 
//   * only software action sinks and software conditions are supported.
//   * no LATE/EARLY/CONFLICT/DELAYED flags are set
struct SoftwareECA {
	struct SearchCandidate {
		SearchCandidate(int first, uint64_t search) : first_walker(first), search_event(search) {
			if (first_walker == 0xffff) {
				first_walker = -1;
			}
		}
		int      first_walker;
		uint64_t search_event;
	};

	std::vector<SearchCandidate> search_candidates;


	struct Search {
		uint64_t id;
		uint64_t mask;
		int first;
		Search(uint64_t _id, uint64_t _mask, int _first) : id(_id), mask(_mask), first(_first) {}
		bool operator==(const Search &rhs) {
			return /*id == rhs.id && mask == rhs.mask &&*/ first == rhs.first;
		}
		Search& operator=(const Search &rhs) {
			id = rhs.id; mask = rhs.mask; first = rhs.first;
			return *this;
		}
	};




	static uint64_t get_time_ns() {
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		uint64_t ns = now.tv_nsec;
		ns += now.tv_sec*UINT64_C(1000000000);
		return ns;
	}

	void inject() {
		if (verbosity >= 1) {
			std::cout << ">>>>>>>>>>>>>>  EVENT INJECTED <<<<<<<<<<<<<<<" << std::endl;
		}
		uint64_t id = in_buffer[0];
		id <<= 32;
		id |= in_buffer[1];
		uint64_t param = in_buffer[2];
		param <<= 32;
		param |= in_buffer[3];
		uint64_t deadline = in_buffer[6];
		deadline <<= 32;
		deadline |= in_buffer[7];
		if (verbosity >= 1) {
			std::cout << "search.size() = " << searches.size() << std::endl;
			std::cout << "walkers:" << std::endl;
		}
		for (auto &walk: walker) {
			walk.second.fired = false;
			if (verbosity >= 1) {
				std::cout << "  " << std::dec << walk.first << " tag = " << walk.second.tag << "  next = " << walk.second.next << std::endl;
			}
		}
		for (auto &search: searches) {
			if (verbosity >= 1) {
				std::cout << "id = " << std::hex << id << " search.id = " << std::hex << search.id << " search.mask = " << std::hex << search.mask << std::endl;
			}
			if ((id&search.mask) == (search.id&search.mask)) {
				int walker_idx = search.first;
				do {
					if (verbosity >= 1) {
						std::cout << "Condition matches walker " << std::dec << walker_idx << " with tag=" << std::dec << walker[walker_idx].tag << std::endl;
					}
					uint64_t offset_deadline = deadline+walker[walker_idx].offset;
					uint32_t msi_data = ECA_VALID<<16; 
					////// late flag doesn't work yet. TODO: make it work!
					// uint64_t now = get_time_ns();
					// if (offset_deadline < now) {
					// 	msi_data = ECA_LATE<<16; 
					// }

					if (walker[walker_idx].fired == false) {
						Event new_event(id,param,offset_deadline,walker[walker_idx].num,walker[walker_idx].tag, msi_target_adr, msi_data|walker[walker_idx].num);
						std::lock_guard<std::mutex> lock(events_mutex);
						events.push_back(new_event);
						std::sort(events.begin(), events.end());
						if (verbosity >= 1) {
							std::cout << "create event with id=" << std::hex << id << " param=" << param << " deadline=" << deadline << std::dec << " num=" << walker_idx << " tag=" << walker[walker_idx].tag << std::endl;
							std::cout << "event.size() = " << events.size() << std::endl;
						}
					}
					walker[walker_idx].fired = true;
					walker_idx = walker[walker_idx].next;
				} while((0xffff&walker_idx) != 0xffff);
			}
		}
	}
	std::vector<Search> searches;


	bool is_prefix_mask(uint64_t mask) {
		if (mask == 0) return true;
		for (uint64_t m = UINT64_C(0xffffffffffffffff); m != 0; m <<= 1) {
			if (mask == m) return true;
		}
		return false;
	}
	uint64_t fix_mask(uint64_t mask) {
		for (uint64_t test = UINT64_C(0x8000000000000000); test != 0; test >>= 1) {
			if ((mask&test) == 0) return ~((test<<1)-1);
		}
		return mask;
	}
	void push_search(const Search& s) {
		if (std::find(searches.begin(), searches.end(), s) == searches.end()) {
			searches.push_back(s);
			if (verbosity >= 1) {
				std::cout << " pushed -> search: " << std::hex << searches.back().id << " " << searches.back().mask << " " << std::dec << searches.back().first;
			}
		} else {
			if (verbosity >= 1) {
				std::cout << " NOT pushed -> search: " << std::hex << s.id << " " << s.mask << " " << std::dec << s.first;
			}
		}

	}

	// this transformation is completely based on heuristics and observation, reverse-engineering and guesswork.
	// it may not be 100% correct but works for a lot of testc cases. TODO: make more test cases
	// (this one is an implementation in case all sowftware conditions have nonzero masks)
	void analyse_search_candidates_normal() {
		uint64_t s_open = 0;
		for (unsigned i = 0; i < search_candidates.size(); ++i) {
			int prev_i = (i>0)?(i-1):(0);
			int      w0 = search_candidates[prev_i].first_walker;
			int      w1 = search_candidates[i].first_walker;
			uint64_t s0 = search_candidates[prev_i].search_event;
			uint64_t s1 = search_candidates[i].search_event;
			if (w1 != -1 && w0 == -1) {
				s_open = s1;
			}
			uint64_t mask1 = ~(s1-s0-1);
			uint64_t id0 = s0&mask1;
			uint64_t id1 = s1&mask1;

			uint64_t mask2 = ~(s1-s_open-1);
			uint64_t id_open  = s_open&mask2;

			if (verbosity >= 1) {
				std::cout << std::dec << w1 << " " << std::hex << s1
				          << " => mask1=" << std::hex <<std::setw(16)<<std::setfill('0') << mask1 
				          << " => mask2=" << std::hex <<std::setw(16)<<std::setfill('0') << mask2 
				          << " is_prefix_mask(" << is_prefix_mask(mask1) << ") w1=" << w1 << " id1=" << id1
				          << " id_open=" << id_open;
			}

			if (is_prefix_mask(mask1) && i) {
				if (w0 == -1) w0 = 0;
				     if (id0) push_search(Search(id0,mask1,w0));
				else if (id1) push_search(Search(id1,mask1,w0));	
			} else if (is_prefix_mask(mask2) && i) {
				if (w0 == -1) w0 = 0;
		    	if (id_open) push_search(Search(id_open,mask2,w0));
			}
			if (verbosity >= 1) {
				std::cout << std::endl;
			}
		}
	}

	// this transformation is completely based on heuristics and observation, reverse-engineering and guesswork.
	// it may not be 100% correct but works for a lot of testc cases. TODO: make more test cases
	// (this one is a special implementation in case a software condition is looking for id=0 with mask=0)
	void analyse_search_candidates_zero() {
		uint64_t s_open = 0;
		for (unsigned i = 0; i < search_candidates.size(); ++i) {
			int prev_i = (i>0)?(i-1):(0);
			int      w0 = search_candidates[prev_i].first_walker;
			int      w1 = search_candidates[i].first_walker;
			uint64_t s0 = search_candidates[prev_i].search_event;
			uint64_t s1 = search_candidates[i].search_event;
			if (w1 != 0 && w0 == 0) {
				s_open = s1;
			}
			uint64_t mask1 = ~(s1-s0-1);
			// uint64_t id0 = s0&mask1;
			uint64_t id1 = s1&mask1;

			uint64_t mask2 = ~(s1-s_open-1);
			uint64_t id_open  = s_open&mask2;

			if (verbosity >= 1) {
				std::cout << std::dec << w1 << " " << std::hex << s1
				          << " => mask1=" << std::hex <<std::setw(16)<<std::setfill('0') << mask1 
				          << " => mask2=" << std::hex <<std::setw(16)<<std::setfill('0') << mask2 
				          << " is_prefix_mask(" << is_prefix_mask(mask1) << ") w1=" << w1 << " id1=" << id1
				          << " id_open=" << id_open;
			}

			if (i == 0) {
				push_search(Search(0,0,w1));
			} else {
				if (w0 != 0) {
					if (w1 > w0) {
						if (is_prefix_mask(mask1) && id1) {
							push_search(Search(id1,mask1,w1));
						} 
					} else {
						if (is_prefix_mask(mask2) && id_open) {
							push_search(Search(id_open,mask2,w0));
						}
					}
				}
			}
			if (verbosity >= 1) {
				std::cout << std::endl;
			}
		}
	}


	void analyse_search_candidates() {
		if (search_candidates.size() /*&& search_candidates[0].first_walker == 0*/ && search_candidates[0].search_event == 0) {
			analyse_search_candidates_zero();
		} else {
			analyse_search_candidates_normal();
		}
		if (verbosity >= 1) {
			std::cerr << ">>>>> searches >>>> " << std::endl;
			for (unsigned i = 0; i < searches.size(); ++i) {
				std::cout << "   -> search: " << std::hex << searches[i].id << " " << searches[i].mask << " " << std::dec << searches[i].first << std::endl;
			}
		}
	}

	struct Walker {
		int64_t offset;
		int32_t  tag;
		int      next;
		int      flags;
		int      channel;
		int      num;
		bool     fired;
	};
	std::map<int, Walker > walker;
	uint32_t in_buffer[8];
	uint32_t msi_target_adr;
	struct Event {
		uint64_t id;
		uint64_t param;
		uint64_t deadline;
		uint32_t num;
		int32_t tag;
		uint32_t msi_adr;
		uint32_t msi_dat;
		Event(uint64_t _id, uint64_t _param, uint64_t _deadline, uint32_t _num, int32_t _tag, uint32_t _msi_adr, uint32_t _msi_dat) : id(_id), param(_param), deadline(_deadline), num(_num), tag(_tag), msi_adr(_msi_adr), msi_dat(_msi_dat) {}
		bool operator==(const Event &rhs) const {
			return id == rhs.id && param == rhs.param && deadline == rhs.deadline /*&& num == rhs.num*/ && tag == rhs.tag && msi_adr == rhs.msi_adr && msi_dat == rhs.msi_dat;
		}
		bool operator<(const Event& rhs) const {
			return deadline < rhs.deadline;
		}
	};
	std::mutex events_mutex;
	std::mutex actions_mutex;
	std::deque<Event> events;
	std::deque<Event> actions; // 2nd thread converts events into actions


	static void eca_events_to_actions(SoftwareECA *software_eca) {
		for(; !FpgaReset::_reset_was_triggered; usleep(100)) {
			for (;;) {
				uint64_t now = SoftwareECA::get_time_ns();
				std::lock_guard<std::mutex> lock_events(software_eca->events_mutex);
				if (!software_eca->events.empty() && software_eca->events.front().deadline <= now) {
					if (verbosity >= 1) {
						std::cout << "creating action: now=" << std::dec << now << " deadline=" << software_eca->events.front().deadline << std::endl;
					}
					std::lock_guard<std::mutex> lock_actions(software_eca->actions_mutex);
					// take an event from the event queue and insert it into 
					//   action queue where it can be read from the ECA_QUEUE Device
					software_eca->actions.push_back(software_eca->events.front());
					software_eca->events.pop_front();
					// create the MSI to signal host that an action is pending
					eb_slave->push_msi(software_eca->actions.back().msi_adr, software_eca->actions.back().msi_dat);
				} else {
					break;
				}
			}
		}
	}

} software_eca; // directly create a global instance of our SoftwareECA


// This class mimics the ECA control registers.
//  When saftlib::TimingReceiver is writing to these registers, the content is 
//  interpreted and the Conditions (event-id, mask, offset) are decoded and 
//  stored in the global instance of the SoftwareECA class.
class EcaUnitControl : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0xb2afc251,
	};
	EcaUnitControl(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 1) {
			std::cout << "EcaUnitControl " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return (adr >= _adr_first) && (adr < _adr_first + 0x100); 
	}

	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		const int channel_type[2] = {0,0};
		const int channel_raw_max_num[2] = {20, 20};
		const int channel_raw_capacity[2] = {100, 100};
		uint32_t result;
		switch(adr-_adr_first) {
			case ECA_CHANNELS_GET:         result =     2;
			break;
			case ECA_SEARCH_CAPACITY_GET:  result = 0x200;
			break;
			case ECA_WALKER_CAPACITY_GET:  result = 0x100;
			break;
			case ECA_LATENCY_GET: result = 12;
			break;
			case ECA_OFFSET_BITS_GET: result = 32;
			break;
    		case ECA_CHANNEL_TYPE_GET:     result = channel_type[_selected_channel];
    		break;
    		case ECA_CHANNEL_MAX_NUM_GET:  result = channel_raw_max_num[_selected_channel];
    		break;
    		case ECA_CHANNEL_CAPACITY_GET: result = channel_raw_capacity[_selected_channel];
    		break;
    		case ECA_TIME_HI_GET: result = SoftwareECA::get_time_ns()>>32;
    		break;
    		case ECA_TIME_LO_GET: result = SoftwareECA::get_time_ns()&0xffffffff;
    		break;
			case ECA_CHANNEL_MOSTFULL_ACK_GET:   result = 0;
			break;
			case ECA_CHANNEL_MOSTFULL_CLEAR_GET: result = 0;
			break;
			case ECA_CHANNEL_VALID_COUNT_GET:    result = 0;
			break;
			case ECA_CHANNEL_OVERFLOW_COUNT_GET: result = 0;
			break;
			case ECA_CHANNEL_FAILED_COUNT_GET:   result = 0;
			break;
    		default: return false;
		}
		if (verbosity >= 1) {
			std::cout << "Eca control read access at " << std::hex << adr << " = " << std::dec << result << std::endl;
		}
		*dat_out = result;
		return true;
	}

	bool is_prefix_mask(uint64_t mask) {
		if (mask == 0) return true;
		for (uint64_t m = UINT64_C(0xffffffffffffffff); m != 0; m <<= 1) {
			if (mask == m) return true;
		}
		return false;
	}
	uint64_t fix_mask(uint64_t mask) {
		for (uint64_t test = UINT64_C(0x8000000000000000); test != 0; test >>= 1) {
			if ((mask&test) == 0) return ~((test<<1)-1);
		}
		return mask;
	}

	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		static int      selected_walker;
		static int      prev_first_walker, first_walker;
		static uint64_t prev_search_event, search_event;
		static bool     end_of_search;
		static bool     select_first;
		// static int      null_walker = 0xffff;
		switch(adr-_adr_first) {
			case ECA_SEARCH_SELECT_RW:
				// std::cerr << "ECA_SEARCH_SELECT_RW " << std::hex << dat << std::endl;
				select_first = false;
				if (dat == 0) {
					if (verbosity >= 1) {
						std::cout << ">>>>>>>>clear software_eca searches" << std::endl;
					}
					software_eca.searches.clear();
					software_eca.search_candidates.clear();
					prev_first_walker = -1;
					prev_search_event = 0;
					select_first = true;
					end_of_search = false;
				} 
				return true;
			case ECA_SEARCH_RW_FIRST_RW:     
				// std::cerr << "ECA_SEARCH_RW_FIRST_RW:" << std::hex << dat << std::endl ;
				first_walker = dat;
				if (first_walker == 0xffff) first_walker = -1;
				return true;
			case ECA_SEARCH_RW_EVENT_HI_RW: 
				// std::cerr << "ECA_SEARCH_RW_EVENT_HI_RW: " << std::hex << dat << std::endl; 
				search_event   = dat; 
				search_event <<= 32;
				return true;
			case ECA_SEARCH_RW_EVENT_LO_RW: 
				// std::cerr << "ECA_SEARCH_RW_EVENT_LO_RW: " << std::hex << dat << std::endl; 
				// search_rw_event |= dat;
				search_event |= dat;


				// insert the data written into the registers into a search-candiate-list
				if (prev_search_event == search_event && prev_first_walker == first_walker && !select_first) {
					if (!end_of_search) {
						// analyze the search-candidate-list and transform it into a list of tuples (id,mask,first_walker)
						software_eca.analyse_search_candidates();
					}
					end_of_search = true;
				}
				if (!end_of_search) {
					software_eca.search_candidates.push_back(SoftwareECA::SearchCandidate(first_walker,search_event));
				}

				prev_first_walker = first_walker;
				prev_search_event = search_event;
				return true;

	    	case ECA_CHANNEL_SELECT_RW: 
	    		if (verbosity >= 1) {
		    		std::cout << "selected channel: " << dat << std::endl;
	    		}
				if (_selected_channel >= 0 || _selected_channel <= 1) {
					_selected_channel = dat; 
					return true;
				} else {
					return false;
				}
			case ECA_CHANNEL_MSI_SET_ENABLE_OWR: return true;
			case ECA_CHANNEL_MSI_SET_TARGET_OWR: 
				eca_msi_target_adr = dat; 
				software_eca.msi_target_adr = dat;
				return true;
			case ECA_WALKER_SELECT_RW:
				// std::cerr << "walker select " << std::dec << dat << std::endl; 
				// _selected_walker = dat;
				selected_walker = dat;
				if (selected_walker == 0) {
					software_eca.walker.clear();
				} 
				return true;
			case ECA_WALKER_RW_NEXT_RW:         
				// std::cerr << "ECA_WALKER_RW_NEXT_RW " << std::hex << dat << std::endl;
				software_eca.walker[selected_walker].next = dat;
				 return true;
			case ECA_WALKER_RW_OFFSET_HI_RW:    
				// std::cerr << "ECA_WALKER_RW_OFFSET_HI_RW " << std::hex << dat << std::endl; 
				software_eca.walker[selected_walker].offset = dat;
				software_eca.walker[selected_walker].offset <<= 32;
				return true;
			case ECA_WALKER_RW_OFFSET_LO_RW:    
				software_eca.walker[selected_walker].offset |= dat;
				//std::cerr << "ECA_WALKER_RW_OFFSET_LO_RW " << std::hex << dat << std::endl; 
				return true;
			case ECA_WALKER_RW_TAG_RW:
				// std::cerr << "walker TAG : " << std::dec << dat << std::endl ;
				software_eca.walker[selected_walker].tag = dat;
				return true;
			case ECA_WALKER_RW_FLAGS_RW:        
				//std::cerr << "ECA_WALKER_RW_FLAGS_RW " << std::hex << dat << std::endl; 
				software_eca.walker[selected_walker].flags = dat;
				return true;
			case ECA_WALKER_RW_CHANNEL_RW:      
				//std::cerr << "ECA_WALKER_RW_CHANNEL_RW " << std::hex << dat << std::endl; 
				software_eca.walker[selected_walker].channel = dat;
				return true;
			case ECA_WALKER_RW_NUM_RW:          
				//std::cerr << "ECA_WALKER_RW_NUM_RW " << std::hex << dat << std::endl; 
				software_eca.walker[selected_walker].num = dat;
				return true;
			case ECA_WALKER_WRITE_OWR:          
				//std::cerr << "write walker tags " << std::hex << dat << std::endl; /*_walker_tags = _walker_tags_tmp; _walker_tags_tmp.clear();*/ 
				if (verbosity >= 1) {
					std::cout << "walker " << std::dec << selected_walker 
								<< " next=" << software_eca.walker[selected_walker].next 
								<< " tag=" << software_eca.walker[selected_walker].tag
								<< " flags=" << std::hex << software_eca.walker[selected_walker].flags
								<< " channel=" << std::dec << software_eca.walker[selected_walker].channel
								<< " num=" << std::dec << software_eca.walker[selected_walker].num
								<< std::endl;
				}
				return true;
		}
		return false; 
	}

private:
	uint32_t _adr_first;
	int      _instance;
	int 	 _selected_channel;
	std::vector<uint32_t> _rom;
};


class EcaQueue : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0xd5a3faea,
	};
	EcaQueue(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 0) {
			std::cout << "EcaQueue " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return (adr >= _adr_first) && (adr < _adr_first + 0x40); 
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {

		// std::cerr << "ECA queue read access " << std::hex << adr-_adr_first << std::endl;
		// std::cerr << "software_eca.events.size()=" << software_eca.events.size() << std::endl;
		std::lock_guard<std::mutex> lock(software_eca.actions_mutex);
		if (!software_eca.actions.empty()) {
			uint64_t execution_time = SoftwareECA::get_time_ns();
			switch (adr-_adr_first) {
				case ECA_QUEUE_QUEUE_ID_GET:    *dat_out = 0; return true;
				case ECA_QUEUE_FLAGS_GET:       *dat_out = (1 << ECA_VALID); return true;
				case ECA_QUEUE_NUM_GET:         *dat_out = software_eca.actions.front().num; return true;
				case ECA_QUEUE_EVENT_ID_HI_GET: *dat_out = software_eca.actions.front().id>>32; /*eca_in_buffer[0]*/; return true;
				case ECA_QUEUE_EVENT_ID_LO_GET: *dat_out = software_eca.actions.front().id&0xffffffff; /*eca_in_buffer[1]*/; return true;
				case ECA_QUEUE_PARAM_HI_GET:    *dat_out = software_eca.actions.front().param>>32; /*eca_in_buffer[2]*/; return true;
				case ECA_QUEUE_PARAM_LO_GET:    *dat_out = software_eca.actions.front().param&0xffffffff; /*eca_in_buffer[3]*/; return true;
				case ECA_QUEUE_TAG_GET:         *dat_out = software_eca.actions.front().tag; /*eca_tag*/;          return true;
				case ECA_QUEUE_TEF_GET:         *dat_out = 0; /*eca_in_buffer[5]*/; return true;
				case ECA_QUEUE_DEADLINE_HI_GET: *dat_out = software_eca.actions.front().deadline>>32;/*eca_in_buffer[6];*/ return true;
				case ECA_QUEUE_DEADLINE_LO_GET: *dat_out = software_eca.actions.front().deadline&0xffffffff;/*eca_in_buffer[7];*/ return true;
				case ECA_QUEUE_EXECUTED_HI_GET: *dat_out = execution_time>>32; return true;
				case ECA_QUEUE_EXECUTED_LO_GET: *dat_out = execution_time&0xffffffff; return true;
			}
		} 
		return false;
	}

	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		std::lock_guard<std::mutex> lock(software_eca.actions_mutex);
		if (adr-_adr_first == ECA_QUEUE_POP_OWR) {
			if (!software_eca.actions.empty()) software_eca.actions.pop_front(); 
			if (verbosity >= 1) {
				std::cout << "ECA_QUEUE_POP_OWR actions.size()=" << software_eca.actions.size() << std::endl; 
			}
			return true;
		}
		return false;
	}

private:
	uint32_t _adr_first;
	int      _instance;
};


class EcaEventsIn : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0x8752bf45,
	};
	EcaEventsIn(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
		, _write_count(0)
	{
		if (verbosity >= 1) {
			std::cout << "EcaEventsIn " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return adr == _adr_first; 
	}
	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		if (verbosity >= 1) {
			std::cout << "eca_event_in " << std::hex << adr << " " << dat << std::endl;
		}
		eca_in_buffer[_write_count] = dat;
		software_eca.in_buffer[_write_count] = dat;
		++_write_count;
		if (_write_count == 8) {
			_write_count = 0;
			if (verbosity >= 1) {
				std::cout << "====> injection of event" << std::endl;
			}
			software_eca.inject();
			//eb_slave->push_msi(eca_msi_target_adr, 0x40000);
		}
		return true;
	}


private:
	uint32_t _adr_first;
	int      _instance;
	int      _write_count;
};

//////////////////////////////////////////////////////////////////
// END OF ECA RELATED CLASSES
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////



class BuildIdRom : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0x2d39fa8b,
	};
	BuildIdRom(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 1) {
			std::cout << "BuildIdRom" << std::endl;
		}
		std::string build_id_text(
			"Project     : software timing receiver\n"
			"Platform    : emulator on host system\n"
			"FPGA model  : -\n"
			"Source info : -\n"
			"Build type  : -\n"
			"Build date  : -\n"
			"Prepared by : -\n"
			"Prepared on : -\n"
			"OS version  : -\n"
			"Quartus     : -\n"
			"\n"
			"  The software timing receiver provides some basic ECA functionality\n"
			"  without the need for real timing hardware.\n"
			"  Events can be injected, SoftwareActionSinks and SoftwareConditoins \n"
			"  can be created using saftlib just as if a hardware timing receiver \n"
			"  was conneced. Don\'t expect high precision or WR-synchronization\n"
			"  Not all saflib drivers will work.\n");

		auto len = build_id_text.length();
		for (unsigned i = 0; i < len; ++i) {
			while (_rom.size() <= i/4) _rom.push_back(0);
			if (i%4 == 0) {
				_rom[i/4] = 0;
			} 
			_rom[i/4] |= build_id_text[i]<<((8*(3-i%4)));
		}
		while (_rom.size() < 0x400) _rom.push_back(0);
	}
	bool contains(uint32_t adr) {
		return (adr >= _adr_first) && (adr < _adr_first + 0x400); 
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		unsigned rom_idx = (adr-_adr_first)/4;
		if (rom_idx >= 0 && rom_idx < _rom.size()) {
			*dat_out = _rom[rom_idx];
			return true; 
		}
		return false;
	}
	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		unsigned rom_idx = (adr-_adr_first)/4;
		if (rom_idx >= 0 && rom_idx < _rom.size()) {
			_rom[rom_idx] = dat;
			return true;
		}
		return false; 
	}
private:
	uint32_t _adr_first;
	int      _instance;
	std::vector<uint32_t> _rom;
};

class WrPpsGenerator : public Device {
public:
	enum {
		vendor_id = 0xce42,
		product_id = 0xde0d8ced,
	};
	WrPpsGenerator(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 1) {
			std::cout << "WrPpsGenerator " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return (adr >= _adr_first) && (adr < _adr_first + 0x100); 
	}
	// this pps generator is alywas locked
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		const uint32_t WR_PPS_GEN_ESCR_MASK = 0xc;  //bit 2: PPS valid, bit 3: TS valid
		const uint32_t WR_PPS_GEN_ESCR      = 0x1c; //External Sync Control Register
		if (adr-_adr_first == WR_PPS_GEN_ESCR) {
			*dat_out = WR_PPS_GEN_ESCR_MASK;
			return true;
		}
		return false;
	}

private:
	uint32_t _adr_first;
	int      _instance;
};




class IoControl : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0x10c05791,
	};
	IoControl(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
	{
		if (verbosity >= 1) {
			std::cout << "IoControl " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return (adr >= _adr_first) && (adr < _adr_first + 0x10000); 
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
  		const uint32_t eGPIO_Info   = 0x0104;
  		const uint32_t eLVDS_Info   = 0x0108;
  		const uint32_t eFIXED_Info  = 0x010c;
		switch(adr-_adr_first) {
			case eGPIO_Info:  *dat_out = 0; return true;
			case eLVDS_Info:  *dat_out = 0; return true;
			case eFIXED_Info: *dat_out = 0; return true;
		}

		return false;
	}

private:
	uint32_t _adr_first;
	int      _instance;
};





class MsiMailbox : public Device {
public:
	enum {
		vendor_id = 0x651,
		product_id = 0xfab0bdd8,
	};
	MsiMailbox(uint32_t adr_first, int instance) 
		: _adr_first(adr_first) 
		, _instance(instance) 
		, _write_count(0)
		, _slots(2*128, 0xffffffff)
	{
		if (verbosity >= 1) {
			std::cout << "MsiMailbox " << std::hex << _adr_first << std::endl;
		}
	}
	bool contains(uint32_t adr) {
		return adr >= _adr_first && adr <= _adr_first + 2*128*4;
	}
	bool write_access(uint32_t adr, int sel, uint32_t dat) {
		if (verbosity >= 1) {
			std::cout << "mailbox write access: " << std::hex << adr << " " << dat << std::endl;
		}
		int idx = (adr-_adr_first)/4;
		_slots[idx] = dat;
		if (!(idx%2)) {
			eb_slave->push_msi(adr, dat);
		}
		return true;
	}
	bool read_access(uint32_t adr, int sel, uint32_t *dat_out) {
		*dat_out = _slots[(adr-_adr_first)/4];
		if (verbosity >= 1) {
			std::cout << "mailbox read access: " << std::hex << adr << " " << *dat_out << std::endl;
		}
		return true;
	}


private:
	uint32_t _adr_first;
	int      _instance;
	int      _write_count;
	std::vector<uint32_t> _slots;
};

///////////////////////////////////////////////////////
// Implement reading SDB-records from hardware
///////////////////////////////////////////////////////
#define ETHERBONE_THROWS 1
#include <etherbone.h>

#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>

std::map<uint32_t, uint32_t> sdb_memory;

static uint32_t get_sdb_size(etherbone::Device device, eb_address_t sdb_start_adr) {
	etherbone::Cycle cycle;
	cycle.open(device);
	eb_data_t magic;
	eb_data_t records_version_bustype;
	cycle.read(sdb_start_adr+0, EB_DATAX, &magic);
	cycle.read(sdb_start_adr+4, EB_DATAX, &records_version_bustype);
	cycle.close();

	if (magic != 0x5344422d) {
		throw std::runtime_error("sdb magic number doesn't match");
	}

	return records_version_bustype >> 16;
}

static eb_address_t sdb_record_adr(eb_address_t sdb_start_adr, int index) {
	return sdb_start_adr+(16*index)*4;
}

static std::vector<uint32_t> read_sdb_record(etherbone::Device device, eb_address_t sdb_start_adr, int index) {
	std::vector<eb_data_t> buffer(16);
	std::vector<uint32_t>  result(buffer.size());

	etherbone::Cycle cycle;
	cycle.open(device);
	
	for (unsigned i = 0; i < buffer.size(); ++i) {
		cycle.read(sdb_record_adr(sdb_start_adr,index)+i*4, EB_DATAX, &buffer[i]);
	}
	cycle.close();

	for (unsigned i=0; i < buffer.size(); ++i) {
		result[i]=buffer[i];
	}
	return result;
}

static void read_sdb(std::map<uint32_t, std::pair<uint32_t, uint32_t> > &result, etherbone::Device device, eb_address_t sdb_start_adr, eb_address_t base_adr = 0x0) {
	// std::cerr << "read sdb at " << std::hex << sdb_start_adr << std::endl;
	uint32_t records = get_sdb_size(device, sdb_start_adr);
	for (unsigned index = 0; index < records; ++index) {
		auto sdb = read_sdb_record(device, sdb_start_adr, index);
		for (unsigned i = 0; i < sdb.size(); ++i) { 
			uint32_t modified_adr = sdb[i];
			if (i == 3 || i == 5) modified_adr += base_adr;
			result[sdb_record_adr(sdb_start_adr,index)+i*4] = std::make_pair(sdb[i], modified_adr);
			// least significant bit of field 15 contains information about sdb record (0x2 means it is a bridge)
			if ((i == 15) && ((sdb[i]&0xff) == 0x2)) {
				read_sdb(result, device, base_adr+sdb[1], base_adr+sdb[3]); // read recursively
			}
		}
	}
}

static void extract_sdb(const char *devicename) {
	etherbone::Socket socket;
	socket.open();

	etherbone::Device device;
	device.open(socket, devicename);

	eb_data_t sdb_start_adr;
	etherbone::Cycle cycle;
	cycle.open(device);
	cycle.read_config(0xc, EB_DATAX, &sdb_start_adr);
	cycle.close();

	// the value has a literal value and a modified value
	// where the literal vale is what was read from the hardware device
	// while the modified value is the same value plus the recursively 
	// added offset address of the interconnect
	std::map<uint32_t, std::pair<uint32_t, uint32_t> > memory;
	read_sdb(memory, device, (eb_address_t)sdb_start_adr);
	for(auto &pair: memory) {
		std::cout << std::hex << std::setw(8) << std::setfill('0') << pair.first;
		std::cout << " => ";
		std::cout << std::hex << std::setw(8) << std::setfill('0') << pair.second.first;
		std::cout << " => ";
		std::cout << std::hex << std::setw(8) << std::setfill('0') << pair.second.second;
		std::cout << std::endl;
	}
}

///////////////////////////////////////////////////////
// End of SDB-record reading
///////////////////////////////////////////////////////



// The main program reads options and does two things:
//  1: If the --extract-sdb <devicename> option is used, try to connect to the specified device
//     and read the SDB records. Then write the SDB-record file to stdout.
//  2: If no option is given, read an SDB record file ("sdb.txt") and register and place some 
//     devices into the address space as specified by the SDB records. Then launch the Etherbone
//     slave to wishbone bridge and redirect read and write request to the devices based on the 
//     wishbone address. The program ends on any write to the FpgaReset device.

int main(int argc, char *argv[]) {

	try {

		std::string sdb_filename = DATADIR "/saftlib/saft-software-tr.sdb";
		if (argc != 1) {
			for (int i = 1; i < argc; i++) {
				std::string argvi(argv[i]);
				if (argvi == "--extract-sdb") {
					++i;
					if (i < argc) {
						// end the program after successful extration of sdb-records
						extract_sdb(argv[i]); 
						return 0;
					} else {
						std::cerr << "expecting device name after --extract-sdb" << std::endl;
						return 1;
					}
				}
				if (argvi == "--sdb") {
					++i;
					if (i < argc) {
						// end the program after successful extration of sdb-records
						sdb_filename = argv[i]; 
					} else {
						std::cerr << "expecting sdb record filename after --sdb" << std::endl;
						return 1;
					}
				}
				if (argvi == "--verbose" || argvi == "-v") {
					++verbosity;
				}
				if (argvi == "--quiet" || argvi == "-q") {
					--verbosity;
				}
			}
		}

		// all devices
		std::vector<std::shared_ptr<Device> > devices;

		// read the sdb-rom from file
		auto sdb = std::make_shared<SDBrecords>(sdb_filename);
		devices.push_back(sdb);
		for(auto &device: sdb->create_devices<WatchdogMutex>() ) devices.push_back(device);
		for(auto &device: sdb->create_devices<BuildIdRom>()    ) devices.push_back(device);
		for(auto &device: sdb->create_devices<WrPpsGenerator>()) devices.push_back(device);
		for(auto &device: sdb->create_devices<EcaUnitControl>()) devices.push_back(device);
		for(auto &device: sdb->create_devices<IoControl>()     ) devices.push_back(device);
		for(auto &device: sdb->create_devices<EcaQueue>()      ) devices.push_back(device);
		for(auto &device: sdb->create_devices<FpgaReset>()     ) devices.push_back(device);
		for(auto &device: sdb->create_devices<EcaEventsIn>()   ) devices.push_back(device);
		for(auto &device: sdb->create_devices<MsiMailbox>()    ) devices.push_back(device);

		// create the etherbone slave.
		// It appears as pseudo-terminal device as /dev/pts/<n>
		//  <n> is an integer selected by the OS
		bool stop_until_connected = true;
		eb_slave = new EBslave(stop_until_connected, 
		                       sdb->start_adr(), 
		                       0x0000, 
		                       0xffff);

		// Endless loop to service wb-requests from the etherbone slave (= wb master)
		// (The code looks a bit strange because it was initially developed with the 
		//  use case of a VHDL simulation in mind.)
		std_logic_t cyc, stb, we;
		std_logic_t ack=STD_LOGIC_0;
		std_logic_t err=STD_LOGIC_0;
		int adr, dat, sel;
		uint32_t dat_out;

		std::thread eca_thread(SoftwareECA::eca_events_to_actions, &software_eca);

		int ending_countdown = 3; // after reset, 3 more eb_slave cycles are needed to end the eb_master transaction
		while(FpgaReset::_reset_was_triggered == false || ending_countdown > 0){
			if (FpgaReset::_reset_was_triggered) {
				eb_slave->shutdown();
				std::cerr << --ending_countdown << std::endl;
			}
			eb_slave->master_out(&cyc,&stb,&we,&adr,&dat,&sel);
			if (cyc==STD_LOGIC_1 && stb==STD_LOGIC_1) {
				bool found_adr = false;
				for (auto &device: devices) {
					// look if any of the devices contains the requested address
					if (device->contains(adr)) {
						found_adr = true;
						bool device_ack = false;
						// do a read/write access on the device
						if (we== STD_LOGIC_1) {
							device_ack = device->write_access(adr,sel,dat);
						} else {
							device_ack = device->read_access(adr,sel,&dat_out);
						}
						if (device_ack) {
							ack = STD_LOGIC_1;
							err = STD_LOGIC_0;
						} else {
							err = STD_LOGIC_1;
							ack = STD_LOGIC_0;
						}
						break;
					}
				} 
				if (!found_adr) {
					if (verbosity >= 0) {
						std::cout << "address not mapped: 0x" 
					              << std::hex << std::setw(8) << std::setfill('0') << adr
						          << std::endl;
					}
					ack = STD_LOGIC_0;
					err = STD_LOGIC_1;
				}
			}

			eb_slave->master_in(stb,  ack, err, STD_LOGIC_0, dat_out);
		}
		eca_thread.join();
		delete eb_slave;
	} catch (std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return false;
	} catch (etherbone::exception_t &e) {
		std::cerr << "Etherbone Error: " << e << std::endl;
	}

	return 0;
}

