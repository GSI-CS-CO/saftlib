/*  Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

// #include <config.h>
#include "Time.hpp"

#include <assert.h>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <exception>

namespace saftlib
{

	const int64_t offset_epoch_01_01_1900 = 2208988800; // obtained with 'date --date='UTC 01/01/1900' +%s'

	// Numbers taken from https://www.ietf.org/timezones/data/leap-seconds.list
	// Subtract the second offset of 01/01/1900 to go to the WhiteRabbit-epoch which starts 01/01/1970
	// The WR-epoch is called "TAI" in the context of this library
	const int64_t leap_second_list[][2] = {
		// add new leap seconds on top of this list
		{UINT64_C(3692217600), 37}, // 1 Jan 2017
		{UINT64_C(3644697600), 36}, // 1 Jul 2015
		{UINT64_C(3550089600), 35}, // 1 Jul 2012
		{UINT64_C(3439756800), 34}, // 1 Jan 2009
		{UINT64_C(3345062400), 33}, // 1 Jan 2006
		{UINT64_C(3124137600), 32}, // 1 Jan 1999
		{UINT64_C(3076704000), 31}, // 1 Jul 1997
		{UINT64_C(3029443200), 30}, // 1 Jan 1996
		{UINT64_C(2982009600), 29}, // 1 Jul 1994
		{UINT64_C(2950473600), 28}, // 1 Jul 1993
		{UINT64_C(2918937600), 27}, // 1 Jul 1992
		{UINT64_C(2871676800), 26}, // 1 Jan 1991
		{UINT64_C(2840140800), 25}, // 1 Jan 1990
		{UINT64_C(2776982400), 24}, // 1 Jan 1988
		{UINT64_C(2698012800), 23}, // 1 Jul 1985
		{UINT64_C(2634854400), 22}, // 1 Jul 1983
		{UINT64_C(2603318400), 21}, // 1 Jul 1982
		{UINT64_C(2571782400), 20}, // 1 Jul 1981
		{UINT64_C(2524521600), 19}, // 1 Jan 1980
		{UINT64_C(2492985600), 18}, // 1 Jan 1979
		{UINT64_C(2461449600), 17}, // 1 Jan 1978
		{UINT64_C(2429913600), 16}, // 1 Jan 1977
		{UINT64_C(2398291200), 15}, // 1 Jan 1976
		{UINT64_C(2366755200), 14}, // 1 Jan 1975
		{UINT64_C(2335219200), 13}, // 1 Jan 1974
		{UINT64_C(2303683200), 12}, // 1 Jan 1973
		{UINT64_C(2287785600), 11}, // 1 Jul 1972
		{UINT64_C(2272060800), 10}, // 1 Jan 1972 
		{-1,11} // last entry. initialize all bits to '1'. 
		        // The offset value is identical tor the first 'real' offset
	};

	std::vector<std::array<int64_t, 2> > leap_second_vector;

	void init(const char* leap_second_list_filename)
	{
		if (!leap_second_vector.empty()) {
			return;
		}
		if (leap_second_list_filename == nullptr) {
			// no filename given
			const char * default_filename = DATADIR "/leap-seconds.list";
			// first: look environment variable
			leap_second_list_filename = getenv("SAFTLIB_LEAPSECONDSLIST");
			if (leap_second_list_filename == nullptr) {
				leap_second_list_filename = default_filename;
			} else {
				std::ifstream testifstream(leap_second_list_filename);
				if (!testifstream.good()) {
					// second: look at standard location
					leap_second_list_filename = default_filename;
				}
			}
		}
		bool invalid_leap_second_list = false;
		// parse the leap seconds list file
		std::ifstream lsin(leap_second_list_filename);
		if (lsin.good()) {
			for (;;) {
				std::string line;
				std::getline(lsin, line);
				if (!lsin) {
					break;
				}
				std::istringstream lin(line);

				char comment;
				lin >> comment;
				if (comment == '#') {
					continue;
				} 

				lin.putback(comment);
				std::array<int64_t, 2> timestamp_offset;
				lin >> timestamp_offset[0] >> timestamp_offset[1];
				if (!lin) {
					break;
				}
				leap_second_vector.push_back(timestamp_offset);
			}
			std::reverse(leap_second_vector.begin(),
				         leap_second_vector.end());
			// add end-of-vector indicator
			leap_second_vector.push_back(leap_second_vector.back());
			leap_second_vector.back()[0] = -1;
		} else { // file couldn't be read... use hard coded list
			invalid_leap_second_list = true;
		}

		if (leap_second_vector.empty()) {
			invalid_leap_second_list = true;
		}
		if (invalid_leap_second_list) {
			std::ostringstream msg;
			msg << "ERROR: saftlib didn't find leap-second.list file at location: \"" << leap_second_list_filename << "\"" << std::endl;
			throw std::runtime_error(msg.str());
		}
	}


	int64_t leap_second_epoch(int n)
	{
		if (leap_second_vector.empty()) {
			init();
		}
		if (leap_second_vector[n][0] == -1) {
			return -1;
		}

		// distinguish negative and positive leap seconds to achieve the correct behavior:
		if (leap_second_vector[n+1][1] < leap_second_vector[n][1]) {
			// positive leap second, the last UTC-second of the day is repeated
			return leap_second_vector[n][0] - offset_epoch_01_01_1900 + leap_second_vector[n][1] - 1;
		}

		// negative leap second, the last UTC-second of the day is skipped
		// remark: Under this treatment, the positive leap second would appear at the first second of the new day.
		//         If this is the desired behavior for positive leap seconds, remove the special treatment for
		//         positive leap seconds (remove the if statement above)
		return leap_second_vector[n][0] - offset_epoch_01_01_1900 + leap_second_vector[n][1] ; 
	}
	int64_t leap_second_offset(int n) 
	{
		if (leap_second_vector.empty()) {
			init();
		}
		return leap_second_vector[n][1];
	}

	int64_t UTC_offset_TAI(uint64_t TAI)
	{
		uint64_t TAIsec = TAI/UINT64_C(1000000000);
		for (int i = 0; ; ++i) { 
			if ((int64_t)TAIsec >= leap_second_epoch(i) || leap_second_epoch(i+1) == -1) {
				return leap_second_offset(i)*UINT64_C(1000000000);
			}
		}
	}

	uint64_t TAI_to_UTC(uint64_t TAI)
	{
		return TAI-UTC_offset_TAI(TAI);
	}

	int TAI_is_UTCleap(uint64_t TAI)
	{
		uint64_t TAIsec = TAI/UINT64_C(1000000000);
		for (int i = 0; ; ++i) { 
			if ((int64_t)TAIsec >= leap_second_epoch(i)) {
				if ((int64_t)TAIsec == leap_second_epoch(i)) {
					return leap_second_offset(i)-leap_second_offset(i+1);
				}
				return 0;
			}
			if (leap_second_epoch(i) == -1) {
				return 0;
			}
		}
	}


	int UTC_offset_UTC(uint64_t UTC, int leap, int64_t *offset)
	{
		uint64_t UTCsec = UTC/UINT64_C(1000000000);
		for (int i = 0; ; ++i) {
			uint64_t TAIsec = UTCsec + leap_second_offset(i);
			if ((int64_t)TAIsec >= leap_second_epoch(i) || leap_second_epoch(i+1) == -1) {
				if ((int64_t)TAIsec == leap_second_epoch(i) && 
					leap_second_offset(i+1) < leap_second_offset(i)) { 
					// If the resulting TAI is on the leap_second_list, the conversion is ambiguous 
					// In this case the 'leap' argument is used to distinguish the two possible TAI values
					// The second part of the condition limits this case to the positive leap seconds
					if (leap) { 
						// the later of the two identical UTC seconds
						*offset = leap_second_offset(i)*UINT64_C(1000000000);
						return 1;
					}
					// the earlier of the two identical UTC seconds
					*offset = leap_second_offset(i+1)*UINT64_C(1000000000);
					return 1;
				}
				*offset = leap_second_offset(i)*UINT64_C(1000000000);
				if (TAI_to_UTC(UTC + *offset) == UTC) {
					return 1;
				}
				return 0;
			}
		}
	}

	int UTC_to_TAI(uint64_t UTC, int leap, uint64_t *TAI)
	{
		int64_t offset;
		if (UTC_offset_UTC(UTC, leap, &offset)) {
			*TAI = UTC + offset;
			return 1;
		}
		return 0;
	}


	int UTC_difference(uint64_t UTC1, int leap1, uint64_t UTC2, int leap2, int64_t *difference)
	{
		uint64_t TAI1, TAI2;
		if (UTC_to_TAI(UTC1, leap1, &TAI1) && UTC_to_TAI(UTC2, leap2, &TAI2)) {
			*difference = (int64_t)TAI1 - (int64_t)TAI2;
			return 1;
		}
		return 0;
	}


	void test_UTC_offset()
	{
		for (int i = 0; ;++i) {
			if (leap_second_epoch(i) == -1) {
				break;
			}
			uint64_t TAI = leap_second_epoch(i)*UINT64_C(1000000000);
			uint64_t UTC = TAI_to_UTC(TAI);
			int64_t offset;
			UTC_offset_UTC(UTC, TAI_is_UTCleap(TAI), &offset);
			assert(offset == leap_second_offset(i)*INT64_C(1000000000));
		}
	}

	void test_UTC_difference()
	{
		for (int i = 0; ;++i) {
			if (leap_second_epoch(i) == -1) {
				break;
			}
			uint64_t TAI = leap_second_epoch(i)*UINT64_C(1000000000);
			uint64_t UTC = TAI_to_UTC(TAI);
			int64_t difference;
			UTC_difference(UTC,1,  UTC, 0, &difference);
			difference /= UINT64_C(1000000000);
			assert(difference == (1+TAI_is_UTCleap(TAI))/2);
		}

		for (int i = 0; ;++i) {
			if (leap_second_epoch(i) == -1) {
				break;
			}
			for (int j = 0; ;++j) {
				if (leap_second_epoch(j) == -1) {
					break;
				}

				uint64_t TAIi = leap_second_epoch(i)*UINT64_C(1000000000);
				uint64_t TAIj = leap_second_epoch(j)*UINT64_C(1000000000);
				uint64_t UTCi = TAI_to_UTC(TAIi);
				uint64_t UTCj = TAI_to_UTC(TAIj);
				int64_t difference = 0;

				UTC_difference(UTCi,1,  UTCj, 1, &difference);
				difference /= UINT64_C(1000000000);
				int64_t tai_difference = (TAIi - TAIj)/INT64_C(1000000000);
				int64_t utc_difference = (UTCi - UTCj)/INT64_C(1000000000);

				assert(difference == tai_difference);
				assert(difference == utc_difference-(leap_second_offset(j)-leap_second_offset(i)));
			}
		}
	}

	void test_conversion_forth_and_back() 
	{
		for (int i = 0; i < 28; ++i) {
			// second scale
			uint64_t TAI0 = leap_second_epoch(i);
			for (uint64_t t = TAI0-10; t < TAI0+10; ++t) {
				uint64_t TAI = t*UINT64_C(1000000000);
				uint64_t UTC = TAI_to_UTC(TAI);
				uint64_t TAI2;
				UTC_to_TAI(UTC, TAI_is_UTCleap(TAI), &TAI2);
				assert(TAI==TAI2);
			}
			// nanosecond scale
			TAI0 = leap_second_epoch(i)*UINT64_C(1000000000);
			for (uint64_t t = TAI0-10; t < TAI0+10; ++t) {
				uint64_t TAI = t;
				uint64_t UTC = TAI_to_UTC(TAI);
				uint64_t TAI2;
				UTC_to_TAI(UTC, TAI_is_UTCleap(TAI), &TAI2);
				assert(TAI==TAI2);
			}
		}
	}

	void test_special_cases()
	{
		uint64_t UTC0 = (leap_second_epoch(20)-leap_second_offset(20))*UINT64_C(1000000000);
		uint64_t UTC_start = UTC0 + UINT64_C(500000000);
		uint64_t UTC_stop  = UTC_start;
	    //compute UTC_stop - UTC_start;
	    int64_t difference;
	    if (UTC_difference(UTC_stop, 1, UTC_start, 0, &difference)) {
	    	// printf("%ld - %ld  =  %ld\n", UTC_stop, UTC_start, difference);
	    	assert(difference == INT64_C(1000000000));
	    } else {
	    	assert(0); // not a valid UTC value
	    }
	    if (UTC_difference(UTC_stop, 0, UTC_start, 1, &difference)) {
	    	// printf("%ld - %ld  =  %ld\n", UTC_stop, UTC_start, difference);
	    	assert(difference == INT64_C(-1000000000));
	    } else {
	    	assert(0); // not a valid UTC value
	    }
	}




	
	Time operator+(const Time& lhs, const int64_t& rhs) {
		Time result(lhs);
		return result += rhs;
	}
	Time operator-(const Time& lhs, const int64_t& rhs) {
		Time result(lhs);
		return result -= rhs;
	}
	Time operator+(const int64_t& lhs, const Time& rhs) {
		return rhs + lhs;
	}
	Time operator-(const int64_t& lhs, const Time& rhs) {
		return rhs - lhs;
	}

	Time makeTimeUTC(uint64_t UTC, bool isLeap) {
		uint64_t TAI;
		if (UTC_to_TAI(UTC, isLeap, &TAI)) {
			return Time(TAI);
		} 
		throw Time(0);
	}
	Time makeTimeTAI(uint64_t TAI) {
		return Time(TAI);
	}
}
