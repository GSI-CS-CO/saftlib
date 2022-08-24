#ifndef SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_
#define SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_

#include <iostream>
#include <iomanip>
#include <string>
#include <cstddef>
#include <cassert>
#include <cstdlib>
#include <cstdio>

namespace saftbus
{

class ChunckAllocatorRT {
	friend class Allocator;
public:
	ChunckAllocatorRT(size_t max_chuncks, size_t chuncksize);
	~ChunckAllocatorRT();
	char* malloc(size_t size);
	void free(char* ptr);
	void print_size();
	void print_state();
	bool contains(char *ptr);
	bool full();
	bool fits(size_t n);
private:
	const size_t MAX_CHUNCKS;
	const size_t CHUNCKSIZE;
	char   *chuncks;
	size_t *backindices;
	size_t *indices;
	size_t allocated_chuncks;
};

class Allocator {
public:
	Allocator();
	char* malloc(size_t n);
	void free(char *ptr);
	std::string fillstate();
private:
	size_t num_allocators;
	ChunckAllocatorRT **allocators;
	size_t heap_allocations;
};

Allocator *get_allocator();
// void *operator new(std::size_t n) {
//   return get_allocator()->malloc(n);
// }
// void operator delete(void *p) {
//   char *ptr = reinterpret_cast<char*>(p);
//   get_allocator()->free(ptr);
// }

}
#endif

