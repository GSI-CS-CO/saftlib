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

#include "Time.hpp"

#include <assert.h>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <exception>

// The WR-epoch is called "TAI" in the context of this library

namespace saftlib
{
	// Epoch conversion constant: seconds between 01/01/1900 and 01/01/1970
	const uint64_t EPOCH_OFFSET_1900_TO_1970 = 2'208'988'800;

	struct LeapSecond {
    	int64_t timestamp;
    	int8_t cumulative_seconds;
	};

	// Reference: IETF Leap Seconds List
	// Source: https://www.ietf.org/timezones/data/leap-seconds.list
	// Last updated: [Okt-2025]
	const std::vector<LeapSecond> LEAP_SECONDS_TABLE {
		{3'692'217'600, 37}, // 1 Jan 2017
		{3'644'697'600, 36}, // 1 Jul 2015
		{3'550'089'600, 35}, // 1 Jul 2012
		{3'439'756'800, 34}, // 1 Jan 2009
		{3'345'062'400, 33}, // 1 Jan 2006
		{3'124'137'600, 32}, // 1 Jan 1999
		{3'076'704'000, 31}, // 1 Jul 1997
		{3'029'443'200, 30}, // 1 Jan 1996
		{2'982'009'600, 29}, // 1 Jul 1994
		{2'950'473'600, 28}, // 1 Jul 1993
		{2'918'937'600, 27}, // 1 Jul 1992
		{2'871'676'800, 26}, // 1 Jan 1991
		{2'840'140'800, 25}, // 1 Jan 1990
		{2'776'982'400, 24}, // 1 Jan 1988
		{2'698'012'800, 23}, // 1 Jul 1985
		{2'634'854'400, 22}, // 1 Jul 1983
		{2'603'318'400, 21}, // 1 Jul 1982
		{2'571'782'400, 20}, // 1 Jul 1981
		{2'524'521'600, 19}, // 1 Jan 1980
		{2'492'985'600, 18}, // 1 Jan 1979
		{2'461'449'600, 17}, // 1 Jan 1978
		{2'429'913'600, 16}, // 1 Jan 1977
		{2'398'291'200, 15}, // 1 Jan 1976
		{2'366'755'200, 14}, // 1 Jan 1975
		{2'335'219'200, 13}, // 1 Jan 1974
		{2'303'683'200, 12}, // 1 Jan 1973
		{2'287'785'600, 11}, // 1 Jul 1972
		{2'272'060'800, 10}, // 1 Jan 1972
		{-1,11} // last entry. kept to guarantee compaibility to legacy functions below
	};

	int64_t leap_second_epoch(int n)
	{
		if (LEAP_SECONDS_TABLE[n].cumulative_seconds == -1) {
			return -1;
		}

		// distinguish negative and positive leap seconds to achieve the correct behavior:
		if (LEAP_SECONDS_TABLE[n+1].cumulative_seconds < LEAP_SECONDS_TABLE[n].cumulative_seconds) {
			// positive leap second, the last UTC-second of the day is repeated
			return LEAP_SECONDS_TABLE[n].cumulative_seconds - EPOCH_OFFSET_1900_TO_1970 + LEAP_SECONDS_TABLE[n].cumulative_seconds - 1;
		}

		// negative leap second, the last UTC-second of the day is skipped
		// remark: Under this treatment, the positive leap second would appear at the first second of the new day.
		//         If this is the desired behavior for positive leap seconds, remove the special treatment for
		//         positive leap seconds (remove the if statement above)
		return LEAP_SECONDS_TABLE[n].cumulative_seconds - EPOCH_OFFSET_1900_TO_1970 + LEAP_SECONDS_TABLE[n].cumulative_seconds ;
	}

	int64_t leap_second_offset(int n)
	{
		return LEAP_SECONDS_TABLE[n].cumulative_seconds;
	}

	int64_t UTC_offset_TAI(uint64_t TAI)
	{
		uint64_t TAIsec = TAI/UINT64_C(1000000000);
		for (int i = 0; ; ++i) {
			if ((int64_t)TAIsec >= leap_second_epoch(i) || leap_second_epoch(i+1) == -1) {
				return leap_second_offset(i)*UINT64_C(1000000000);
			}
		}
		return TAIsec;
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

	//
	// test functions follow
	// 
	void test_UTC_offset()
	{
		for (size_t i = 0; i <  LEAP_SECONDS_TABLE.size();++i) {
			uint64_t TAI = leap_second_epoch(i)*UINT64_C(1000000000);
			uint64_t UTC = TAI_to_UTC(TAI);
			int64_t offset;
			UTC_offset_UTC(UTC, TAI_is_UTCleap(TAI), &offset);
			assert(offset == leap_second_offset(i)*INT64_C(1000000000));
		}
	}

	void test_UTC_difference()
	{
		for (size_t i = 0; i <  LEAP_SECONDS_TABLE.size();++i) {
			uint64_t TAI = leap_second_epoch(i)*UINT64_C(1000000000);
			uint64_t UTC = TAI_to_UTC(TAI);
			int64_t difference;
			UTC_difference(UTC,1,  UTC, 0, &difference);
			difference /= UINT64_C(1000000000);
			assert(difference == (1+TAI_is_UTCleap(TAI))/2);
		}

		for (int i = 0; LEAP_SECONDS_TABLE.size();++i) {

			for (size_t j = 0; LEAP_SECONDS_TABLE.size();++j) {

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
		for (size_t i = 0; i < LEAP_SECONDS_TABLE.size(); ++i) {
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
}
