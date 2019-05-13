#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <iostream>
#include <iomanip>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <inttypes.h>

// modes for printing to cout
const uint32_t PMODE_NONE     = 0x0;
const uint32_t PMODE_DEC      = 0x1;
const uint32_t PMODE_HEX      = 0x2;
const uint32_t PMODE_VERBOSE  = 0x4;

// formatting of mask for action sink
uint64_t     tr_mask(int i                    	  // number of bits
                    );

// formatting of date for output
std::string tr_formatDate(uint64_t time,      	  // time [ns]
                          uint32_t pmode      	  // mode for printing
                          );

// formatting of action event ID for output
std::string tr_formatActionEvent(uint64_t id,  	  // 64bit event ID
                                 uint32_t pmode	  // mode for printing
                                 );

// formatting of action param for output; the format depends also on evtNo, except if evtNo == 0xFFFF FFFF
std::string tr_formatActionParam(uint64_t param,    // 64bit parameter
                                 uint32_t evtNo,    // evtNo (currently 12 bit) - part of the 64 bit event ID
                                 uint32_t pmode     // mode for printing
                                 );

// formatting of action flags for output
std::string tr_formatActionFlags(uint16_t flags,    // 16bit flags
                                 uint64_t delay,    // used in case action was delayed
                                 uint32_t pmode     // mode for printing
                                 );

#endif /* #ifndef COMMON_FUNCTIONS_H */
