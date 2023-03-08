#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SoftwareActionSink_Proxy.hpp>
#include <SoftwareCondition_Proxy.hpp>

#include <CommonFunctions.h>

#include <memory>
#include <iostream>

void on_action(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
  std::cout << "event " << event << " " 
            << "param " << param << " " 
            << "deadline " << deadline.getTAI() << " "
            << "executed " << executed.getTAI() << " " 
            << "flags " << flags << " "
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <saftlib-device> <id> <mask> <param>" << std::endl;
    return 0;
  }

  uint64_t snoop_args[3]; 
  uint64_t &id    = snoop_args[0];
  uint64_t &mask  = snoop_args[1];
  uint64_t &param = snoop_args[2];
  for (int i = 2; i < 5; ++i) {
    std::istringstream argin(argv[i]);
    argin >> snoop_args[i-2];
    if (!argin) {
      std::cerr << "cannot read snoop argument from " << argv[i] << std::endl;
      return 1;
    }
  }

  auto saftd = saftlib::SAFTd_Proxy::create();
  auto tr = saftlib::TimingReceiver_Proxy::create(saftd->getDevices()[argv[1]]);
  auto softwareActionSink_obj_path = tr->NewSoftwareActionSink("");
  auto softwareActionSink          = saftlib::SoftwareActionSink_Proxy::create(softwareActionSink_obj_path);
  auto condition_obj_path          = softwareActionSink->NewCondition(true, id, mask, param);
  auto sw_condition                = saftlib::SoftwareCondition_Proxy::create(condition_obj_path);

  sw_condition->setAcceptEarly(true);
  sw_condition->setAcceptLate(true);
  sw_condition->setAcceptDelayed(true);
  sw_condition->setAcceptConflict(true);

  sw_condition->SigAction.connect(sigc::ptr_fun(&on_action));

  for (;;) {
    //saftbus::Loop::get_default().iteration(true); 
    saftlib::wait_for_signal();
    // saftbus::SignalGroup::get_global().wait_for_signal();
  }

  return 0;
}
