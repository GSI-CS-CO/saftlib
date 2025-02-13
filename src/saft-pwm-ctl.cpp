/* Synopsis */
/* ==================================================================================================== */
/* SCU bus interface control application */

/* Defines */
/* ==================================================================================================== */
#include <cstdint>
#include <memory>
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

/* Includes */
/* ==================================================================================================== */
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"

/* Namespaces */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Globals */
/* ==================================================================================================== */
static const char *deviceName = NULL;   /* Name of the device */
static const char *program    = NULL;   /* Name of the application */

/* Prototypes */
/* ==================================================================================================== */
static void pwm_ctl_help (void);
static int  pwm_ctl_set (uint8_t channel, uint32_t frequency, uint16_t duty_cycle);
static int  pwm_ctl_list (void);
static int  pwm_ctl_delete (uint8_t channel);
static bool pwm_is_input_valid(uint8_t channel, uint32_t frequency, uint16_t duty_cycle);

/* Function main() */
/* ==================================================================================================== */
int main (int argc, char** argv)
{
  /* Helpers */
  //char     *pEnd             = NULL;
  int      opt               = 0;
  int      return_code       = 0;

  uint8_t   channel          = 0;
  uint32_t  frequency        = 0;
  uint16_t  duty_cycle       = 0;

  bool      set_channel      = false;
  bool      del_channel      = false;
  bool      list_pwm         = false;
  bool      show_help        = false;

  /* Get the application name */
  program = argv[0]; 

  /* Get the device name */
  deviceName = argv[1];

   /* Parse for options */
  while ((opt = getopt(argc, argv, "c:d:lvh")) != -1)
  {
    switch (opt)
    {
      /* add input validation! */
      case 'c': { set_channel      = true;
                  if (argv[optind-1] != NULL) { channel = stoi(argv[optind-1]); }
                  else                        { std::cerr << "Error: Missing value for channel number!" << std::endl; return (-1); }
                  if (argv[optind-0] != NULL) { frequency = stoi(argv[optind-0]); }
                  else                        { std::cerr << "Error: Missing value for frequency[Hz]!" << std::endl; return (-1); }
                  if (argv[optind+1] != NULL) { duty_cycle = stoi(argv[optind+1]); }
                  else                        { std::cerr << "Error: Missing value for duty cycle[%]!" << std::endl; return (-1); }
                  break; }
      case 'd': { del_channel     = true;
                  if (argv[optind-1] != NULL) { channel = stoi(argv[optind-1]); }
                  else                        { std::cerr << "Error: Missing value for channel number!" << std::endl; return (-1); }
                  break; }
      case 'l': { list_pwm         = true; break; }
      case 'h': { show_help        = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; break; }
    }
  }

  /* Help wanted? */
  if (show_help) { pwm_ctl_help(); return (0); }

  /* Get basic arguments, we need at least the device name */
  deviceName = argv[optind];

  /* Try to connect to saftd */
  try 
  {
    /* Check if device name exists */
    if (deviceName == NULL)
    { 
      std::cerr << "Missing device name!" << std::endl;
      return (-1);
    }
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    if (devices.find(deviceName) == devices.end())
    {
      std::cerr << "Device " << deviceName << " does not exist!" << std::endl;
      return (-1);
    }

    /* Proceed with program */
    if ((set_channel)   &&   pwm_is_input_valid(channel, frequency, duty_cycle))  { return_code = pwm_ctl_set(channel, frequency, duty_cycle); }
    else if (del_channel)    { return_code = pwm_ctl_set(channel, frequency, duty_cycle);  }
    else if (del_channel)    { return_code = pwm_ctl_delete(channel);  }
    else if (list_pwm)       { return_code = pwm_ctl_list();  }
    else                     { pwm_ctl_help(); }
    
  }
  catch (const saftbus::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  /* Done */
  return (return_code);
}


/* Function pwm_is_input_valid */
/* ==================================================================================================== */
static bool pwm_is_input_valid(uint8_t channel, uint32_t frequency, uint16_t duty_cycle)
{
  bool is_valid = false;

  if ((channel < 0) || (channel < 9)) { std::cerr << "Error: Channel number out of range!" << std::endl; return is_valid; }
  else if (frequency <= 0) { std::cerr << "Error: Unsound frequency!" << std::endl; return is_valid; }
  else if ((duty_cycle < 1) || (duty_cycle > 99)) { std::cerr << "Error: Unsound duty cycle!" << std::endl; return is_valid; }
  else {is_valid = true;}

  return is_valid;

}


/* Function pwm_ctl_set */
/* ==================================================================================================== */
static int  pwm_ctl_set(uint8_t channel, uint32_t frequency, uint16_t duty_cycle){

  /* Initialize saftlib components */
  
  /* Try to set the PWM */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    receiver->PWM_Test();
  }
  catch (const saftbus::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (-1);
  }

  return 1;

};


/* Function pwm_ctl_list */
/* ==================================================================================================== */
static int  pwm_ctl_list(void){

  /* Initialize saftlib components */
  
  /* Try to set the PWM */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    receiver->PWM_Test();
  }
  catch (const saftbus::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (-1);
  }

  return 1;

};


/* Function pwm_ctl_delete */
/* ==================================================================================================== */
static int  pwm_ctl_delete(uint8_t channel){

  /* Initialize saftlib components */
  
  /* Try to set the PWM */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    receiver->PWM_Test();
  }
  catch (const saftbus::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (-1);
  }

  return 1;

};



/* Function pwm_ctl_help */
/* ==================================================================================================== */
static void pwm_ctl_help(void)
{
  /* Print arguments and options */
  std::cout << "PWM-CTL for SAFTlib" << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -c <channel number(1-8)> <frequency[Hz]> <duty cycle[%]>: Create a new PWM on this channel" << std::endl;
  std::cout << "  -d <channel number(1-8)>:                                 Stop the output on this channel and delete the set values" << std::endl;
  std::cout << "  -l:                                                       List currently set options" << std::endl;
  std::cout << "  -h:                                                       Print help (this message)" << std::endl;
  std::cout << std::endl;
  std::cout << "Note:" << std::endl;
  /* TODO calc freq min max */
  std::cout << "  Channel: [1-8], Frequency: integer, Percentage: integer [1-99]" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << program << " device " << "-c 1 1 70 "<< std::endl;
  std::cout << "  This will create a PWM output on channel 1 with a frequency of 1Hz and a duty cycle of 70%" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <csco-tg@gsi.de>" << std::endl;
  std::cout << "Licensed under the GPLv3" << std::endl;
  std::cout << std::endl;
}
