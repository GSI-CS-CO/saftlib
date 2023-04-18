#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SoftwareActionSink_Proxy.hpp>
#include <SoftwareCondition_Proxy.hpp>
#include <Output_Proxy.hpp>
#include <OutputCondition_Proxy.hpp>
#include <Input_Proxy.hpp>

#include <CommonFunctions.h>

#include <saftbus/error.hpp>

#include <poll.h>

#include <memory>
#include <iostream>
#include <thread>
#include <cassert>

// arbitrary but fixed and unique event id prefix that is used to trigger the transceiver IO
uint64_t id = 0xffff123400000000;  

bool program_active = true;

void tx_setup(std::shared_ptr<saftlib::TimingReceiver_Proxy> tr, const std::string &IoName) 
{
  std::map< std::string, std::string > outs = tr->getOutputs();
  std::shared_ptr<saftlib::Output_Proxy> tx_out = saftlib::Output_Proxy::create(outs[IoName], tr->get_signal_group());

  bool     active = true;; 
  uint64_t mask   = 0xffffffffffffffff; 
  int64_t  offset = 10000000; 
  bool     on;
  auto rising  = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 1, mask, offset, on=true), tr->get_signal_group()); 
  rising->setAcceptDelayed(true);
  auto falling = saftlib::OutputCondition_Proxy::create(tx_out->NewCondition(active, id | 0, mask, offset, on=false), tr->get_signal_group());
  falling->setAcceptDelayed(true);

  // enable output 
  tx_out->setOutputEnable(true);
  // the output driver has to be low, because the output should be driven by ECA action channel.
  // The actucal output is an OR of ECA output action channel, output driver, clock generator.
  tx_out->WriteOutput(false);
  // inject one event to assure that the output is high
  tr->InjectEvent(id | 1, 0, saftlib::makeTimeTAI(0));
}

void tx_send(std::shared_ptr<saftlib::TimingReceiver_Proxy> tr, uint8_t data, uint64_t time_of_one_bit_ns) 
{
  saftlib::Time now = tr->CurrentTime(); 
  tr->InjectEvent(id | 0, 0, now+time_of_one_bit_ns*0); // start bit
  int level = 0;
  for (int i = 1; i <= 8; ++i) {
  	uint64_t mask(0x100 >> i);
  	int edge = (data&mask) ? 1 : 0 ;
    if (edge != level) {
      tr->InjectEvent(id | edge , 0, now+time_of_one_bit_ns*i); // data bits
    }
    level = edge;
  }
  tr->InjectEvent(id | 0, 0, now+time_of_one_bit_ns*9);  // stop bit
  tr->InjectEvent(id | 1, 0, now+time_of_one_bit_ns*10); // end
}

void tx_thread(const std::string &DeviceName, uint32_t bps, const std::string &IoName)
{ 
  saftbus::SignalGroup tx_group;
  uint64_t time_of_one_bit_ns = 1000000000/bps;
  auto saftd = saftlib::SAFTd_Proxy::create();
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[DeviceName], tx_group);
  tx_setup(tr, IoName);

  std::string line;
  while (program_active) {
    struct pollfd pfd;
    pfd.fd = 0;
    pfd.events = POLLIN;
    if (poll(&pfd,1,10)==1) { // only call getline if there is something to read (timeout of 10 ms)
      std::getline(std::cin, line);
      if (!std::cin) break;
      std::istringstream lin(line);
      for (auto ch: line) {
        tx_send(tr, ch, time_of_one_bit_ns);
        // depsite the hardware being capable of pretty high baut rates
        // the total symbol rate on the receiving side is limited by 
        // how fast MSIs are processed by the host. 
        // Errors are prevented by waiting some time between sending symbols.
        usleep(10000+time_of_one_bit_ns/1000*20);
      }
      tx_send(tr, '\n', time_of_one_bit_ns);
    }
  }
}

struct io_catcher {
  saftlib::Time previous_deadline;
  int bit_idx;
  int level;
  uint8_t data;

  io_catcher() {
    previous_deadline = saftlib::makeTimeTAI(0);
    bit_idx = 0;
    level = 0;
    data = 0;
  }

  void io_catch_input(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags, uint64_t time_of_one_bit_ns)
  {
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
      std::mutex m;
      std::lock_guard<std::mutex> l(m);
      std::cout << data << std::flush;
    }
  }

};

void on_condition_destroyed() {
  std::cerr << "on_condition_destroyed" << std::endl;
  program_active = false;
}

void rx_thread(const std::string &DeviceName, uint32_t bps, const std::string &IoName) {
  uint64_t prefix = 0xffffe00000000000;
  saftbus::SignalGroup rx_group;
  auto saftd = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib", rx_group);
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[DeviceName], rx_group);
  auto inputs = tr->getInputs();
  auto input = saftlib::Input_Proxy::create(inputs[IoName], rx_group);
  input->setEventEnable(false);
  input->setEventPrefix(prefix);
  input->setEventEnable(true);

  bool reuse = false;
  try {
    saftlib::SoftwareActionSink_Proxy::create(tr->NewSoftwareActionSink(IoName+"_rx"), rx_group);
  } catch (saftbus::Error &e) {
    std::cerr << "reusing action sink because " << e.what() << std::endl;
    reuse = true;
  }
  auto sink = saftlib::SoftwareActionSink_Proxy::create(tr->getSoftwareActionSinks()[IoName+"_rx"], rx_group);
  try {
    if (!reuse) {
      sink->NewCondition(true, prefix, 0xffffffffffffff00, 10000);
    }
  } catch (saftbus::Error &e) {
    std::cerr << "reusing condition because " << e.what() << std::endl;
  }
  // std::cerr << "sink conditions " << sink->getAllConditions().size() << std::endl;
  while(sink->getAllConditions().size() == 0) {}
  assert(sink->getAllConditions().size() == 1);
  std::string condition_object_path = sink->getAllConditions()[0];
  auto cond = saftlib::SoftwareCondition_Proxy::create(condition_object_path, rx_group);
  io_catcher catcher;
  cond->SigAction.connect(sigc::bind(sigc::mem_fun(&catcher, &io_catcher::io_catch_input), 1000000000/bps));
  cond->Destroyed.connect(sigc::ptr_fun(&on_condition_destroyed));
  while (program_active) {
    rx_group.wait_for_signal(1000);
  }
  std::cerr << "rx_done" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <saftlib-device> <bps> <TX-IO> <RX-IO>" << std::endl;
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
  const int max_bps = 10000000;
  if (bps < min_bps) {
    std::cerr << "minimum bps is " << min_bps << std::endl;
    return 1;
  }
  if (bps > max_bps) {
    std::cerr << "maximum bps is " << max_bps << std::endl;
    return 1;
  }

  std::thread tx(&tx_thread, argv[1], bps, argv[3]);
  std::thread rx(&rx_thread, argv[1], bps, argv[4]);

  tx.join();
  rx.join();
  

  return 0;
}

