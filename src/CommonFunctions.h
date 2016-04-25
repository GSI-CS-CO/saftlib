#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <iostream>
#include <iomanip>
#include <giomm.h>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

guint64     tr_mask(int i);
const char* tr_formatDate(guint64 time);

#endif /* #ifndef COMMON_FUNCTIONS_H */
