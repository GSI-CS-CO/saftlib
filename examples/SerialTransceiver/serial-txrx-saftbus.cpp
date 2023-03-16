#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SoftwareActionSink_Proxy.hpp>
#include <SoftwareCondition_Proxy.hpp>
#include <Output_Proxy.hpp>
#include <OutputCondition_Proxy.hpp>
#include <Input_Proxy.hpp>

#include <CommonFunctions.h>

#include <memory>
#include <iostream>
#include <thread>

// arbitrary but fixed and unique event id prefix that is used to trigger the transceiver IO
uint64_t id = 0xffff123400000000;  

void tx_setup(std::shared_ptr<saftlib::TimingReceiver_Proxy> tr, const std::string &IoName) 
{
  std::map< std::string, std::string > outs = tr->getOutputs();
  std::shared_ptr<saftlib::Output_Proxy> tx_out = saftlib::Output_Proxy::create(outs[IoName]);

  bool     active = false;; 
  uint64_t mask   = 0xffffffffffffffff; 
  int64_t  offset = 0; 
  bool     on;
  auto start  = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0x00, mask, offset, on=false)); // start bit (falling-edge)
  start->setAcceptEarly(true);
  for (int i = 1; i <= 8; ++i) {
    auto rising  = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0x10 | i, mask, offset, on=true)); 
    rising->setAcceptEarly(true);
    auto falling = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0x00 | i, mask, offset, on=false));
    falling->setAcceptEarly(true);
  }
  auto stop = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0x09, mask, offset, on=false)); // stop bit (falling-edge)
  stop->setAcceptEarly(true);
  auto end  = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0x1a, mask, offset, on=true)); // back to high state (rising-edge)
  end->setAcceptEarly(true);

  // configuration of conditions is done -> enable them all (ToggleActive makes all inactive conditions active and vice versa)
  // enable output and inject one event to assure that the output is high
  tx_out->setOutputEnable(true);
  tx_out->ToggleActive();
  tr->InjectEvent(id | 0x1a, 0, saftlib::makeTimeTAI(0)); // inject the end event which makes the output go to high
}

void tx_send(std::shared_ptr<saftlib::TimingReceiver_Proxy> tr, uint8_t data, uint64_t time_of_one_bit_ns) 
{
  saftlib::Time now = tr->CurrentTime(); 
  now += 10000000;
  tr->InjectEvent(id | 0x00, 0, now+time_of_one_bit_ns*0);
  for (int i = 1; i <= 8; ++i) {
  	uint64_t mask(0x100 >> i);
  	int edge = (data&mask) ? 0x10 : 0x00 ;
  	tr->InjectEvent(id | edge | i, 0, now+time_of_one_bit_ns*i); // data bits
  }
  tr->InjectEvent(id | 0x09, 0, now+time_of_one_bit_ns*9);  // stop bit
  tr->InjectEvent(id | 0x1a, 0, now+time_of_one_bit_ns*10); // end
}

void tx_thread(const std::string &DeviceName, uint32_t bps, const std::string &IoName)
{ 
  uint64_t time_of_one_bit_ns = 1000000000/bps;
	auto saftd = saftlib::SAFTd_Proxy::create();
	auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[DeviceName]);
	tx_setup(tr, IoName);

  std::string line;
	for (;;) {
    std::cerr << ">>> ";
    std::getline(std::cin, line);
    if (!std::cin) break;
    std::istringstream lin(line);
    for (auto ch: line) {
		  tx_send(tr, ch, time_of_one_bit_ns);
      usleep(time_of_one_bit_ns/1000*20);
    }
    tx_send(tr, '\n', time_of_one_bit_ns);
	}

}

void io_catch_input(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags, uint64_t time_of_one_bit_ns)
{
  static saftlib::Time previous_deadline = saftlib::makeTimeTAI(0);
  static int bit_idx = 0;
  static int level = 0;
  static uint8_t data = 0;

  uint64_t delta_ns = deadline-previous_deadline;
  previous_deadline = deadline;
  if (bit_idx == 9 || delta_ns > time_of_one_bit_ns*11) {
    bit_idx = -1; // this is the start bit
  } else {
    int num_bits = (delta_ns+time_of_one_bit_ns/2)/time_of_one_bit_ns;
    for (int i = bit_idx; i < bit_idx+num_bits; ++i) {
      if (i<0 || i>7) continue;
      if (level) data |=  (1<<(7-i));
      else       data &= ~(1<<(7-i));
    }
    bit_idx += num_bits;
  }
  level = (event & 0x1)?1:0;
  if (bit_idx == 9) {
    //std::cerr << "received: 0x" << std::hex << (int)data << std::dec << std::endl;
    std::cerr << data;
  }
}

void rx_thread(const std::string &DeviceName, uint32_t bps, const std::string &IoName) {
  // saftbus::SignalGroup group;
  auto saftd = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib");
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[DeviceName]);
  auto inputs = tr->getInputs();
  auto input = saftlib::Input_Proxy::create(inputs[IoName]);
  input->setEventEnable(true);

  auto sink = saftlib::SoftwareActionSink_Proxy::create(tr->NewSoftwareActionSink(IoName+"_rx"));
  auto cond = saftlib::SoftwareCondition_Proxy::create(sink->NewCondition(false, 0xfffe000000000000, 0xffffffffffffff00, 10000));
  cond->setAcceptEarly(true);
  cond->setAcceptLate(true);
  cond->setAcceptDelayed(true);
  cond->setAcceptConflict(true);
  sink->ToggleActive();
  cond->SigAction.connect(sigc::bind(sigc::ptr_fun(&io_catch_input), 1000000000/bps));
  for (;;) {
    saftbus::SignalGroup::get_global().wait_for_signal(1000);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <saftlib-device> <bps> <TX-IO> rx/tx" << std::endl;
    return 0;
  }

  uint32_t bps;
  std::istringstream argin(argv[2]);
  argin >> bps;
  if (!argin) {
    std::cerr << "cannot read pbs from " << argv[2] << std::endl;
    return 1;
  }
  const int min_bps = 10;
  const int max_bps = 10000;
  if (bps < min_bps) {
    std::cerr << "minimum bps is " << min_bps << std::endl;
    return 1;
  }
  if (bps > max_bps) {
    std::cerr << "maximum bps is " << max_bps << std::endl;
    return 1;
  }

  // std::thread tx(&tx_thread, argv[1], bps, argv[3]);
  // std::thread rx(&rx_thread, argv[1], bps, argv[3]);
  
  // sleep(2);
  if (std::string("rx") == argv[4] ) {
    rx_thread(argv[1], bps, argv[3]);
  }
  if (std::string("tx") == argv[4]) {
    tx_thread(argv[1], bps, argv[3]);
  }

  char ch;
  std::cin >> ch;

  return 0;
}
