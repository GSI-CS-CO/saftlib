#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <iostream>
#include <iomanip>
#include <giomm.h>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <inttypes.h>

// modes for printing to cout
const guint32 PMODE_NONE     = 0x0;
const guint32 PMODE_DEC      = 0x1;
const guint32 PMODE_HEX      = 0x2;
const guint32 PMODE_VERBOSE  = 0x4;

// formatting of mask for action sink
guint64     tr_mask(int i                    	  // number of bits
                    );

// formatting of date for output
std::string tr_formatDate(guint64 time,      	  // time [ns]
                          guint32 pmode      	  // mode for printing
                          );

// formatting of action event ID for output
std::string tr_formatActionEvent(guint64 id,  	  // 64bit event ID
                                 guint32 pmode	  // mode for printing
                                 );

// formatting of action param for output; the format depends also on evtNo, except if evtNo == 0xFFFF FFFF
std::string tr_formatActionParam(guint64 param,    // 64bit parameter
                                 guint32 evtNo,    // evtNo (currently 12 bit) - part of the 64 bit event ID
                                 guint32 pmode     // mode for printing
                                 );

// formatting of action flags for output
std::string tr_formatActionFlags(guint16 flags,    // 16bit flags
                                 guint64 delay,    // used in case action was delayed
                                 guint32 pmode     // mode for printing
                                 );

#endif /* #ifndef COMMON_FUNCTIONS_H */
