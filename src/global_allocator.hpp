/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
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
#ifndef SAFTBUS_GLOBAL_ALLOCATOR_HPP_
#define SAFTBUS_GLOBAL_ALLOCATOR_HPP_

#include "chunck_allocator_rt.hpp"

class Allocator {
public:
	Allocator() {
		allocator_1 = new(::malloc(sizeof(ChunckAllocatorRT<16384,128>))) ChunckAllocatorRT<16384,128>;
		allocator_2 = new(::malloc(sizeof(ChunckAllocatorRT<128,1024>))) ChunckAllocatorRT<128,1024>;
		allocator_3 = new(::malloc(sizeof(ChunckAllocatorRT<64,16384>))) ChunckAllocatorRT<64,16384>;
	}
	~Allocator() {
		::free(allocator_3);
		::free(allocator_2);
		::free(allocator_1);
	}
	char* malloc(size_t n) {
		// std::cerr << "--------malloc----------" << std::endl;
		// allocator_1->print_size();
		// allocator_2->print_size();
		// allocator_3->print_size();
					 if (allocator_1->fits(n) && !allocator_1->full()) {
			return allocator_1->malloc(n);
		} else if (allocator_2->fits(n) && !allocator_2->full()) {
			return allocator_2->malloc(n);
		} else if (allocator_3->fits(n) && !allocator_3->full()) {
			return allocator_3->malloc(n);
		} else {
			std::cerr << "HEAP!!!!!!!!!!!!!!!!!!! " << n << std::endl;
			return reinterpret_cast<char*>(::malloc(n));
		}
	}
	void free(char *ptr) {
					 if (allocator_1->contains(ptr)) {
			allocator_1->free(ptr);
		} else if (allocator_2->contains(ptr)) {
			allocator_2->free(ptr);
		} else if (allocator_3->contains(ptr)) {
			allocator_3->free(ptr);
		} else {
			::free(ptr);
		}
		// std::cerr << "--------free------------" << std::endl;
		// allocator_1->print_size();
		// allocator_2->print_size();
		// allocator_3->print_size();
	}
private:
	ChunckAllocatorRT<16384,128> *allocator_1;
	ChunckAllocatorRT<128,1024> *allocator_2;
	ChunckAllocatorRT<64,16384> *allocator_3;
};


static Allocator *get_allocator() {
	static Allocator *allocator = new(::malloc(sizeof(*allocator))) Allocator;
	return allocator;
}
void *operator new(std::size_t n) {
	return get_allocator()->malloc(n);
}
void operator delete(void *p) {
	char *ptr = reinterpret_cast<char*>(p);
	get_allocator()->free(ptr);
}

#endif

