#ifndef SAFTBUS_PROCESS_SETTINGS_HPP_
#define SAFTBUS_PROCESS_SETTINGS_HPP_

#include <string>


// the following functions can be used to change process parameters: scheduling, cpu affinity, io priority
bool set_realtime_scheduling(std::string argvi, char *prio);
bool set_cpu_affinity(std::string argvi, char *affinity);
bool set_ioprio(char *ioprio);


#endif
