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

//////////////////////////////////////////////////////////////////////////////
// Conversion functions between UTC and TAI values 
//   (TAI epoch is 01/01/1970 within this library) 
// All time values are in nanoseconds
// All functions handle negative leap seconds 
//   (in case they will be introduced in the future)
//////////////////////////////////////////////////////////////////////////////

#ifndef SAFTLIB_TIME_H_
#define SAFTLIB_TIME_H_

#include <stdint.h>


namespace saftlib
{
		
	//extern const int64_t leap_second_list[][2];
	int64_t leap_second_epoch(int n);
	int64_t leap_second_offset(int n);

	void init(const char* leap_second_list_filename = nullptr);

	//////////////////////////////////////////////////////////////////////////////
	// convert TAI value to UTC value
	//////////////////////////////////////////////////////////////////////////////
	// parameters: TAI         nanosecond TAI value (nanoseconds since 01/01/1970)
	// returns:    UTC value that corresponds to the given TAI value. Result is
	//             ambiguous when it happens to be in a leap second interval. In 
	//             that case, use the function TAI_is_UTCleap() to disambiguate. 
	int64_t UTC_offset_TAI(uint64_t TAI);
	uint64_t TAI_to_UTC(uint64_t TAI);

	//////////////////////////////////////////////////////////////////////////////
	// check if this TAI value falls into a UTC leap second interval
	//////////////////////////////////////////////////////////////////////////////
	// parameters: TAI         nanosecond TAI value (nanoseconds since 01/01/1970)
	// returns:    1 if this is a positive leap second interval (the UTC second 
	//               part was repeated)
	//             0 if this was outside a leap second interval
	//            -1 if the previous second was skipped (only happens if we get 
	//               negative leap seconds, which didn't happen yet (year 2019))
	int TAI_is_UTCleap(uint64_t TAI);


	//////////////////////////////////////////////////////////////////////////////
	// get UTC offset (in seconds) for given UTC value
	//    UTC_offset = TAI - UTC 
	//////////////////////////////////////////////////////////////////////////////
	// parameters: UTC         nanosecond UTC value
	//             leap        1 if UTC1 is inside a leap second interval, 
	//                         0 otherwise
	//                         (This parameter is only ever looked at, when the 
	//                         UTC value is actually in a leap second interval. 
	//                         For non leap second intervals, this parameter is 
	//                         ignored.)
	//             offset      a pointer to where the resulting offset is copied
	//                         after successful computation (return value 1)
	//                         the offset is given in nanoseconds
	// returns:    1           if the conversion could be done, 
	//             0           if the UTC value value was invalid 
	//                         (nonexistent UTC values will only appear if we get
	//                         negative leap seconds, which didn't happen yet 
	//                         (year 2019))
	int UTC_offset_UTC(uint64_t UTC, int leap, int64_t *offset);

	//////////////////////////////////////////////////////////////////////////////
	// convert UTC value to TAI value
	//////////////////////////////////////////////////////////////////////////////
	// parameters: UTC         nanosecond UTC value
	//             leap        1 if UTC1 is inside a leap second interval, 
	//                         0 otherwise
	//                         (This parameter is only ever looked at, when the 
	//                         UTC value is actually in a leap second interval. 
	//                         For non leap second intervals, this parameter is 
	//                         ignored.)
	//             TAI         a pointer to where the result of the conversion is 
	//                         copied after successful computation
	// returns:    1           if the conversion could be done, 
	//             0           if the UTC value value was invalid 
	//                         (nonexistent UTC values will only appear if we get
	//                         negative leap seconds, which didn't happen yet 
	//                         (year 2019))
	int UTC_to_TAI(uint64_t UTC, int leap, uint64_t *TAI);

	//////////////////////////////////////////////////////////////////////////////
	// calculate UTC time difference
	//////////////////////////////////////////////////////////////////////////////
	// parameters: UTC1        nanosecond UTC value
	//             leap1       1 if UTC1 is inside a leap second interval,
	//                         0 otherwise
	//                         (This parameter is only ever looked at, when the 
	//                         UTC value is actually in a leap second interval. 
	//                         For non leap second intervals, this parameter is 
	//                         ignored.)
	//             UTC2        see UTC1
	//             leap2       see leap1
	//             difference  a pointer to where the number of nanoseconds 
	//                         UTC1-UTC2 is stored (in case of return value 1)
	// returns:    1           if the difference could be calculated, 
	//             0           if one of the UTC values was invalid 
	//                         (nonexistent UTC values will only appear if we get
	//                         negative leap seconds, which didn't happen yet 
	//                         (year 2019))
	int UTC_difference(uint64_t UTC1, int leap1, 
		               uint64_t UTC2, int leap2, 
		               int64_t *difference);








	// some test functions for the library function implementations
	void test_UTC_offset();
	void test_UTC_difference();
	void test_conversion_forth_and_back();
	void test_special_cases();


	const int64_t sec = INT64_C(1000000000);
	const int64_t msec = INT64_C(1000000);
	const int64_t usec = INT64_C(1000);
	const int64_t nsec = INT64_C(1);

	class Time
	{
	public:
		// implicit conversion to uint64_t (do we really want this?)
		// operator uint64_t() {
		// 	return TAI;
		// }
		
	public:
		Time() : TAI(0) {
		}
		Time(const Time& t) : TAI(t.TAI) {
		}
		uint64_t getUTC() const {
			return TAI_to_UTC(TAI);
		}
		uint64_t getTAI() const {
			return TAI;
		}
		Time& operator=(const Time& t) {
			TAI = t.TAI;
			return *this;
		}
		Time& operator+=(const int64_t& duration) {
			TAI += duration;
			return *this;
		}
		Time& operator-=(const int64_t& duration) {
			TAI -= duration;
			return *this;
		}
		int64_t operator-(const Time& rhs) const {
			return TAI - rhs.TAI;
		}
		int64_t getUTCOffset() const {
			return UTC_offset_TAI(TAI);
		}
		bool operator>(const Time& rhs) const {
			return TAI > rhs.TAI;
		}
		bool operator<(const Time& rhs) const {
			return TAI < rhs.TAI;
		}
		bool operator>=(const Time& rhs) const {
			return TAI >= rhs.TAI;
		}
		bool operator<=(const Time& rhs) const {
			return TAI <= rhs.TAI;
		}
		bool operator==(const Time& rhs) const {
			return TAI == rhs.TAI;
		}
		bool operator!=(const Time& rhs) const {
			return TAI != rhs.TAI;
		}
		int isLeapUTC() const {
			return TAI_is_UTCleap(TAI);
		}

		friend Time makeTimeUTC(uint64_t UTC, bool isLeap);
		friend Time makeTimeTAI(uint64_t TAI);
	private:
		explicit Time(const uint64_t& value) : TAI(value) {
		}

		uint64_t TAI;
	};

	Time operator+(const Time& lhs, const int64_t& rhs);
	Time operator-(const Time& lhs, const int64_t& rhs);
	Time operator+(const int64_t& lhs, const Time& rhs);
	Time operator-(const int64_t& lhs, const Time& rhs);
	Time makeTimeUTC(uint64_t UTC, bool isLeap = false);
	Time makeTimeTAI(uint64_t TAI);


}


#endif
