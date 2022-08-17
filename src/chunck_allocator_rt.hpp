#ifndef SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_
#define SAFTBUS_CHUNCK_ALLOCATOR_RT_HPP_

#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cassert>

namespace saftbus {


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
			if (index != chuncks[allocated_chuncks-1].index) 
			{
				size_t &a_idx = indices[chuncks[index].index];
				size_t &b_idx = indices[allocated_chuncks-1];

				size_t &a_backidx = chuncks[a_idx].index;
				size_t &b_backidx = chuncks[b_idx].index;

				// std::cerr << "swapping " << a_idx << " <-> " << b_idx << std::endl;
				std::swap(a_idx, b_idx);
				// std::cerr << "swapping " << a_backidx << " <-> " << b_backidx << std::endl;
				std::swap(a_backidx, b_backidx);
				// check();
			}
			--allocated_chuncks;
		}
		// void operator()(Base *ptr) {
		// 	ptr->~Base();
		// 	this->free((char*)ptr);
		// }
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
	private:
		struct Chunck{
			size_t index;
			alignas(std::max_align_t) char buffer[CHUNCKSIZE];
		};
		Chunck chuncks[MAX_CHUNCKS];
		size_t indices[MAX_CHUNCKS];
		size_t allocated_chuncks;
	};

}

#endif

