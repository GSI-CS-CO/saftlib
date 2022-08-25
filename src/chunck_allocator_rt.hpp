#ifndef SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_
#define SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_

#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cassert>


template<size_t MAX_CHUNCKS, size_t CHUNCKSIZE>
class ChunckAllocatorRT {
public:
	ChunckAllocatorRT() 
		: allocated_chuncks(0)
	{
		for (size_t i = 0; i < MAX_CHUNCKS; ++i) {
			chuncks[i].index = i;
			indices[i]       = i;
		}
	}
	char* malloc(size_t size) {
		assert(size <= CHUNCKSIZE);
		assert(allocated_chuncks < MAX_CHUNCKS);
		// std::cerr << "alloc chunck " << allocated_chuncks << std::endl;
		return chuncks[indices[allocated_chuncks++]].buffer;
	}

	void free(char* ptr) {
		assert(allocated_chuncks > 0);
		assert(contains(ptr));
		size_t index = (ptr-chuncks[0].buffer)/sizeof(Chunck);
		// std::cerr << "freeing chunck " << index << std::endl;

		size_t &a_idx = indices[chuncks[index].index];
		size_t &b_idx = indices[allocated_chuncks-1];

		size_t &a_backidx = chuncks[a_idx].index;
		size_t &b_backidx = chuncks[b_idx].index;

		// std::cerr << "swapping " << a_idx << " <-> " << b_idx << std::endl;
		std::swap(a_idx, b_idx);
		// std::cerr << "swapping " << a_backidx << " <-> " << b_backidx << std::endl;
		std::swap(a_backidx, b_backidx);

		--allocated_chuncks;
	}
	void print_size() {
		std::cerr << "filled: " << allocated_chuncks << "/" << MAX_CHUNCKS << std::endl;
	}

	void print_state() {
		for (int i = 0; i < MAX_CHUNCKS; ++i) {
			std::cerr << std::setw(3) << chuncks[i].index << " ";
		}
		std::cerr << std::endl;
		for (int i = 0; i < MAX_CHUNCKS; ++i) {
			std::cerr << std::setw(3) << indices[i] << " ";
		}
		std::cerr << std::endl;
		for (int i = 0; i < allocated_chuncks; ++i) {
			std::cerr << std::setw(3) << " " << " ";
		}
		std::cerr << "  ^" << std::endl;
	}
	bool contains(char *ptr) {
		return (ptr >= chuncks[0].buffer
			 && ptr <= chuncks[MAX_CHUNCKS].buffer);
	}
	bool full() {
		return allocated_chuncks == MAX_CHUNCKS;
	}
	bool fits(size_t n) {
		return n <= CHUNCKSIZE;
	}
private:
	struct Chunck{
		char buffer[CHUNCKSIZE];
		size_t index;
	};
	alignas(std::max_align_t) Chunck chuncks[MAX_CHUNCKS];
	size_t indices[MAX_CHUNCKS];
	size_t allocated_chuncks;
};

class Allocator {
public:
	Allocator() {
		allocator_1 = new(::malloc(sizeof(ChunckAllocatorRT<1024,128>))) ChunckAllocatorRT<1024,128>;
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
		std::cerr << "--------free------------" << std::endl;
		allocator_1->print_size();
		allocator_2->print_size();
		allocator_3->print_size();
	}
private:
	ChunckAllocatorRT<1024,128> *allocator_1;
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

