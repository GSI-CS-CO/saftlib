#include "chunck_allocator_rt.h"

namespace saftbus
{

	ChunckAllocatorRT::ChunckAllocatorRT(size_t max_chuncks, size_t chuncksize) 
		: MAX_CHUNCKS(max_chuncks)
		, CHUNCKSIZE(chuncksize)
		, chuncks    (reinterpret_cast<char*>  (::malloc(max_chuncks*chuncksize)))
		, backindices(reinterpret_cast<size_t*>(::malloc(max_chuncks*sizeof(size_t))))
		, indices    (reinterpret_cast<size_t*>(::malloc(max_chuncks*sizeof(size_t))))
		, allocated_chuncks(0)
	{
		for (size_t i = 0; i < MAX_CHUNCKS; ++i) {
			backindices[i] = i;
			indices[i]     = i;
		}
	}
	ChunckAllocatorRT::~ChunckAllocatorRT() {
		print_size();
		::free(indices);
		::free(backindices);
		::free(chuncks);
	}
	char* ChunckAllocatorRT::malloc(size_t size) {
		assert(size <= CHUNCKSIZE);
		assert(allocated_chuncks < MAX_CHUNCKS);
		return &chuncks[indices[allocated_chuncks++]*CHUNCKSIZE];
	}

	void ChunckAllocatorRT::free(char* ptr) {
		assert(allocated_chuncks > 0);
		assert(contains(ptr));
		size_t index = (ptr-&chuncks[0])/CHUNCKSIZE;

		size_t &a_idx = indices[backindices[index]];
		size_t &b_idx = indices[allocated_chuncks-1];

		size_t &a_backidx = backindices[a_idx];
		size_t &b_backidx = backindices[b_idx];

		std::swap(a_idx,     b_idx);
		std::swap(a_backidx, b_backidx);

		--allocated_chuncks;
	}
	void ChunckAllocatorRT::print_size() {
		std::cerr << "filled: " << allocated_chuncks << "/" << MAX_CHUNCKS << std::endl;
	}

	void ChunckAllocatorRT::print_state() {
		for (size_t i = 0; i < MAX_CHUNCKS; ++i) {
			std::cerr << std::setw(3) << backindices[i] << " ";
		}
		std::cerr << std::endl;
		for (size_t i = 0; i < MAX_CHUNCKS; ++i) {
			std::cerr << std::setw(3) << indices[i] << " ";
		}
		std::cerr << std::endl;
		for (size_t i = 0; i < allocated_chuncks; ++i) {
			std::cerr << std::setw(3) << " " << " ";
		}
		std::cerr << "  ^" << std::endl;
	}
	bool ChunckAllocatorRT::contains(char *ptr) {
		return (ptr >= &chuncks[0]
			 && ptr <  &chuncks[MAX_CHUNCKS*CHUNCKSIZE]);
	}
	bool ChunckAllocatorRT::full() {
		return allocated_chuncks == MAX_CHUNCKS;
	}
	bool ChunckAllocatorRT::fits(size_t n) {
		return n <= CHUNCKSIZE;
	}


	Allocator::Allocator() {
		const char *allocator_config_string = getenv("SAFTD_ALLOCATOR_CONFIG");
		if (allocator_config_string == NULL) {
			allocator_config_string = "1024.128 128.1024 64.16384 16.262144";
		}
		heap_allocations = 0;
		num_allocators = 0;
		const char* ptr = allocator_config_string;
		if (*ptr) num_allocators = 1;
		while(*ptr) {
			if (*ptr == ' ') ++num_allocators;
			++ptr;
		}
		ptr = allocator_config_string;
		allocators = reinterpret_cast<ChunckAllocatorRT**>(::malloc(num_allocators*sizeof(ChunckAllocatorRT*)));
		for (size_t i = 0; i < num_allocators; ++i) {
			int max_chuncks = 0;
			int chuncksize = 0;
			sscanf(ptr,"%d.%d",&max_chuncks, &chuncksize);
			if (i > 0) {
				assert(static_cast<int>(allocators[i-1]->MAX_CHUNCKS) > max_chuncks); // later allocators must have fewer chuncks
				assert(static_cast<int>(allocators[i-1]->CHUNCKSIZE)  < chuncksize); // later allocators must use larger chuncks
			}
			printf("creating allocator with % 6d chuncks of size % 6d\n", max_chuncks, chuncksize);
			allocators[i] = new(::malloc(sizeof(ChunckAllocatorRT))) ChunckAllocatorRT(max_chuncks,chuncksize);
			while(*ptr) if (*ptr++ == ' ') break; // advance to next pair of numbers
		}
	}
	char* Allocator::malloc(size_t n) {
		for (size_t i = 0; i < num_allocators; ++i) {
			if (allocators[i]->fits(n) && !allocators[i]->full()) {
				return allocators[i]->malloc(n);
			}
		}
		++heap_allocations;
		std::cerr << "heap allocation" << n << std::endl;
		return reinterpret_cast<char*>(::malloc(n));
	}
	std::string Allocator::fillstate() {
		std::ostringstream msg;
		for (size_t i = 0; i < num_allocators; ++i) {
			msg << allocators[i]->CHUNCKSIZE << " : " << allocators[i]->allocated_chuncks << "/" << allocators[i]->MAX_CHUNCKS << std::endl;
		}
		msg << "heap : " << heap_allocations << std::endl;
		return msg.str();
	}

	void Allocator::free(char *ptr) {
		// for (size_t i = 0; i < num_allocators; ++i) {
		// 	allocators[i]->print_size();
		// }
		// std::cerr << "heap: " << heap_allocations << std::endl;
		// std::cerr << "---------" << std::endl;
		for (size_t i = 0; i < num_allocators; ++i) {
			if (allocators[i]->contains(ptr)) {
				allocators[i]->free(ptr);
				return;
			}
		}
		--heap_allocations;
		::free(ptr);
	}


Allocator *get_allocator() {
	static Allocator *allocator = new(::malloc(sizeof(*allocator))) Allocator;
  return allocator;
}


}
// void *operator new(std::size_t n) {
//   return get_allocator()->malloc(n);
// }
// void operator delete(void *p) {
//   char *ptr = reinterpret_cast<char*>(p);
//   get_allocator()->free(ptr);
// }


