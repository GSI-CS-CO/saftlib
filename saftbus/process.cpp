#include "process.hpp"

#include <iostream>
#include <sstream>
#include <cstring>

// for scheduling and cpu affinity settings
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

// for io priority settings
#include <linux/ioprio.h>    /* Definition of IOPRIO_* constants */
#include <sys/syscall.h>     /* Definition of SYS_* constants */
#include <unistd.h>




bool set_realtime_scheduling(std::string argvi, char *prio) {
  std::istringstream in(prio);
  sched_param sp;
  in >> sp.sched_priority;
  if (!in) {
    std::cerr << "Error: cannot read priority from argument " << prio << std::endl;
    return false;
  }
  int policy = SCHED_RR;
  if (argvi == "-f") policy = SCHED_FIFO;
  if (sp.sched_priority < sched_get_priority_min(policy) && sp.sched_priority > sched_get_priority_max(policy)) {
    std::cerr << "Error: priority " << sp.sched_priority << " not supported " << std::endl;
    return false;
  } 
  if (sched_setscheduler(0, policy, &sp) < 0) {
    std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

bool set_cpu_affinity(std::string argvi, char *affinity) {
  // affinity should be a comma-separated list such as "1,4,5,9"
  cpu_set_t set;
  CPU_ZERO(&set);
  std::string affinity_list = affinity;
  for(auto &ch: affinity_list) if (ch==',') ch=' ';
  std::istringstream in(affinity_list);
  int count = 0;
  for (;;) {
    int CPU;
    in >> CPU;
    if (!in) break;
    CPU_SET(CPU, &set);
    ++count;
  }
  if (!count) {
    std::cerr << "Error: cannot read cpus from argument " << affinity << std::endl;
    return false;
  }
  if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
    std::cerr << "Error: failed to set scheduling policy: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

bool set_ioprio(char *ioprio) {
  std::string class_data = ioprio;
  for(auto &ch: class_data) if (ch==',') ch=' ';
  std::istringstream in(class_data);
  int io_prio_class, io_prio_data;
  in >> io_prio_class >> io_prio_data;
  if (!in) {
    std::cerr << "Error: cannot read priority class,data from argument " << ioprio << std::endl;
    return false;
  }
  if (io_prio_class < 0 || io_prio_class > 3) {
    std::cerr << "Error: io prio class must be in range [0 .. 3] " << std::endl;
    return false;
  }
  if (io_prio_data < 0 || io_prio_data > 7) {
    std::cerr << "Error: io prio data must be in range [0 .. 7] " << std::endl;
    return false;
  }
  if (syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, IOPRIO_PRIO_VALUE(io_prio_class, io_prio_data)) < 0) {
    std::cerr << "Error: failed to set io priority: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

