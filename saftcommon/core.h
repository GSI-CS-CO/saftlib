/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef SAFTBUS_CORE_H_
#define SAFTBUS_CORE_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <string.h>


#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <sigc++/sigc++.h>

namespace saftbus 
{

	extern int _debug_level;

	void init();

	void block_signal(int signal_to_block /* i.e. SIGPIPE */ );

	int write_all(int fd, const void *buffer, int size);
	int read_all(int fd, void *buffer, int size);

	int write_all_signal(int fd, const void *buffer, int size);

	int sendfd(int socket, int fd);
	int recvfd(int socket);

	template<typename T>
	int write(int fd, const T & scalar)	{
		int result = write_all(fd, static_cast<const void*>(&scalar), sizeof(scalar));
		return result;
	}
	template<typename T>
	int read(int fd, T & scalar) {
		int result = read_all(fd, static_cast<void*>(&scalar), sizeof(scalar));
		return result;
	}

	// std::set (not nested) 
	template<typename T>
	int write(int fd, const std::set<T>& std_set) {
		uint32_t size = std_set.size();
		int result = write(fd, size);
		if (result == -1) return result;
		for (auto content: std_set ) {
			result = write(fd, content);
			if (result == -1) return result;
		}
		return 1;
	}
	template<typename T>
	int read(int fd, std::set<T>& std_set) {
		uint32_t size;
		int result = read(fd, size);
		if (result == -1) return result;
		std_set.clear();
		for (uint32_t i = 0; i < size; ++i) {
			T content;
			result = read(fd, content);
			if (result == -1) return result;
			std_set.insert(content);
		}
		return 1;
	}

	// std::vectors and nested std::vectors
	template<typename T>
	int write(int fd, const std::vector<T>& std_vector) {
		if (_debug_level > 5) std::cerr << "non-nested vector write" << std::endl;
		uint32_t size = std_vector.size();
		int result = write_all(fd, static_cast<void*>(&size), sizeof(uint32_t));
		if (result == -1) return result;
		if (size > 0) return write_all(fd, static_cast<const void*>(&std_vector[0]), size*sizeof(decltype(std_vector.back())));
		return 1;
	}
	template<typename T>
	int write(int fd, const std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
		if (_debug_level > 5) std::cerr << "nested vector write" << std::endl;
		uint32_t size = std_vector_vector.size();
		int result = write_all(fd, static_cast<const void*>(&size), sizeof(uint32_t));
		if (result == -1) return result;
		for (uint32_t i = 0; i < size; ++i) {
			result = write(fd, std_vector_vector[i]);
			if (result == -1) return result;
		}
		return 1; // meaning success
	}
	template<typename T>
	int read(int fd, std::vector<T> & std_vector) {
		if (_debug_level > 5) std::cerr << "vector read" << std::endl;
		uint32_t size;
		int result = read_all(fd, static_cast<void*>(&size), sizeof(uint32_t));
		if (result == -1) {
			return result;
		}
		std_vector.resize(size);
		if (size > 0) {
			return read_all(fd, static_cast<void*>(&std_vector[0]), size*sizeof(decltype(std_vector.back())));
		}
		return 1;
	}
	template<typename T>
	int read(int fd, std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
		if (_debug_level > 5) std::cerr << "nested vector read" << std::endl;
		uint32_t size;
		int result = read_all(fd, static_cast<void*>(&size), sizeof(uint32_t));
		if (result == -1) {
			return result;
		}
		std_vector_vector.resize(size);
		for (uint32_t i = 0; i < size; ++i) {
			result = read(fd, std_vector_vector[i]);
			if (result == -1) return result;
		}
		return 1; // meaning success
	}
	// specialize for std::string and std::string
	template<>
	int write<std::string>(int fd, const std::string & std_vector);
	template<>
	int read<std::string>(int fd, std::string & std_vector);
	template<>
	int write<std::string>(int fd, const std::string & std_vector);
	template<>
	int read<std::string>(int fd, std::string & std_vector);

	// std::maps of arbitrary type
	template<class K, class V>
	int write(int fd, const std::map<K,V> &map) {
		if (_debug_level > 5) std::cerr << "map write" << std::endl;
		uint32_t size = map.size();
		int result = write(fd, size);
		if (result == -1) return result;
		for (auto iter = map.begin(); iter != map.end(); ++iter) {
			result = write(fd, iter->first); 
			if (result == -1) return result;
			result = write(fd, iter->second);
			if (result == -1) return result;
		}
		return 1; // meaning success
	}
	template<class K, class V>
	int read(int fd, std::map<K,V> &map) {
		if (_debug_level > 5) std::cerr << "map read" << std::endl;
		map.clear();
		uint32_t size;
		int result = read(fd, size);
		if (result == -1) return result;
		K k;  V v;
		for (uint32_t i = 0; i < size; ++i) {
			result = read(fd, k); 
			if (result == -1) return result;
			result = read(fd, v);
			if (result == -1) return result;
			map[k] = v;
		}
		return 1; // meaning success
	}


	template<class Tfirst, class ... Tother>
	int write(int fd, Tfirst first, Tother ... other) {
		if (_debug_level > 5) std::cerr << "tuple write" << std::endl;
		if (write(fd, first) == -1) return -1;
		if (write(fd, other ...) == -1) return -1;
		return 1;
	}

	template<class Tfirst, class ... Tother>
	int read(int fd, Tfirst first, Tother ... other) {
		if (_debug_level > 5) std::cerr << "tuple read" << std::endl;
		if (read(fd, first) == -1) return -1;
		if (read(fd, other ...) == -1) return -1;
		return 1;
	}


	// Simple class for serialization and de-serialization 
	// without storing type information, i.e. de-serialization 
	// only works if the type composition is known (but this 
	// is the case in all saftbus transfers)
	class Serial
	{
	public:
		Serial()
		{
			_iter = _data.begin();
		}
		void put_init()
		{
			_data.clear();
		}
		// has to be called before any call to get()
		void get_init(const std::vector<char> &data) 
		{
			_data = data;
			get_init();
		}
		// has to be called before any call to get()
		void get_init() const
		{
			_iter = _data.begin();
		}
		// POD struct and build-in types
		template<typename T>
		void put(const T &val) {
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&val));
			const char* end   = begin + sizeof(val);
			_data.insert(_data.end(), begin, end);
		}
		template<typename T>
		void get(T &val) const {
			val    = *const_cast<T*>(reinterpret_cast<const T*>(&(*_iter)));
			_iter += sizeof(val);
		}
		// std::vector and nested std::vector
		template<typename T>
		void put(const std::vector<T>& std_vector) {
			size_t size = std_vector.size();
			put(size);
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_vector[0]));
			const char* end   = begin + size*sizeof(std_vector[0]);
			_data.insert(_data.end(), begin, end);
		}
		template<typename T>
		void put(const std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
			size_t size = std_vector_vector.size();
			put(size);
			for (size_t i = 0; i < size; ++i) {
				put(std_vector_vector[i]);
			}
		}
		template<typename T>
		void get(std::vector<T> &std_vector) const {
			size_t size;
			get(size);
			const T* begin = const_cast<T*>(reinterpret_cast<const T*>(&(*_iter)));
			const T* end   = begin + size;
			std_vector.clear();
			std_vector.insert(std_vector.end(), begin, end);
			_iter += sizeof(T)*size;
		}
		template<typename T>
		void get(std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) const {
			size_t size;
			get(size);
			std_vector_vector.resize(size);
			for (size_t i = 0; i < size; ++i) {
				get(std_vector_vector[i]);
			}
		}
		// std::string 
		void put(const std::string& std_string) {
			size_t size = std_string.size();
			put(size);
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_string[0]));
			const char* end   = begin + size*sizeof(std_string[0]);
			_data.insert(_data.end(), begin, end);
		}
		void get(std::string &std_string) const {
			size_t size;
			get(size);
			//std::cerr << " got size " << size << std::endl;
			const char* begin = &(*_iter);
			const char* end   = begin + size;
			std_string.clear();
			std_string.insert(std_string.end(), begin, end);
			_iter += size;
		}
		// std::vector<std::string>
		void put(const std::vector<std::string>& vector_string) {
			size_t size = vector_string.size();
			put(size);
			for (size_t i = 0; i < size; ++i) {
				put(vector_string[i]);
			}
		}
		void get(std::vector<std::string> &vector_string) const {
			size_t size;
			get(size);
			vector_string.resize(size);
			for (size_t i = 0; i < size; ++i) {
				get(vector_string[i]);
			}
		}
		// std::map
		template<typename K, typename V>
		void put(const std::map<K,V> &std_map) {
			size_t size = std_map.size();
			put(size);
			for (typename std::map<K,V>::const_iterator it = std_map.begin(); it != std_map.end(); ++it) {
				put(it->first);
				put(it->second);
			}
		}
		template<typename K, typename V>
		void get(std::map<K,V> &std_map) {
			std_map.clear();
			size_t size;
			get(size);
			for (size_t i = 0; i < size; ++i) {
				K key;
				V value;
				get(key);
				get(value);
				std_map.insert(std::make_pair(key,value));
			}
		}
		// nested Serials
		void put(const Serial &ser) {
			put(ser._data);
		}
		void get(Serial &ser) const {
			get(ser._data);
		}

		std::vector<char>& data(){
			return _data;
		}
		const char* get_data() const {
			return &_data[0];
		}
		unsigned get_size() const {
			return _data.size();
		}
		void print() const {
			std::cerr << _data.size() << std::endl;
			for (auto ch: _data) {
				std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)ch << std::dec << std::endl;
			}
		}
	private:
		std::vector<char> _data;
		mutable std::vector<char>::const_iterator _iter;
	};




// 	// WriteBuffer class provides a flat memory area for serialization 
// 	// of high-level data types. The memory is managed like a ring buffer
// 	// that grows on demand. It can dump the serialized data into a file
// 	// descriptor using a function that is provided via a function pointer 
// 	// template parameter. It assumes that the function signature is like 
// 	// the POSIX write function. Write functions with the same signature 
// 	// can be provided for testing purposes. 
// 	typedef ssize_t write_fn_type(int fd, const void *buf, size_t count);
// 	template<write_fn_type write_fn>
// 	class WriteBuffer
// 	{
// 	public:
// 		// initialize with a file descriptor
// 		WriteBuffer(int fd, unsigned initial_size = 16) : _fd(fd), _w_idx(0), _r_idx(0), _buffer(initial_size)	{
// 			// make sure that _fd is non blocking
// 			if (write_fn == &::write) { // only if POSIX write function is used 
// 				int flags = fcntl(_fd, F_GETFL, 0);
// 				fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
// 			}
// 		}
// 		WriteBuffer(const WriteBuffer &wb) {
// 			_fd = wb._fd; // file descriptor
// 			_w_idx = wb._w_idx;
// 			_r_idx = wb._r_idx;
// 			_buffer = wb._buffer;			
// 		}

// 		int get_fd() const {return _fd;}
// 		void set_fd(int fd) {_fd = fd;}

// 		// return true if no data is in the ring buffer
// 		bool empty() {
// 			return _w_idx == _r_idx;
// 		}

// 		// return true if 
// 		bool full() {
// 			//*****wr***** full is like this
// 			//r**********w or like this
// 			return (_w_idx == _r_idx-1) ||
// 			       (_w_idx == _buffer.size()-1 && _r_idx == 0);
// 		}

// 		unsigned used() {
// 			if (_w_idx == _r_idx) {
// 				return 0;
// 			} 
// 			if (_w_idx > _r_idx) {
// 				return _w_idx - _r_idx;
// 			}
// 			return _buffer.size() + _w_idx - _r_idx;
// 		}
// 		unsigned unused() {
// 			return _buffer.size() - used() - 1;
// 		}

// 		// POD struct and build-in types
// 		template<typename T>
// 		void put(const T &val) {
// 			const unsigned char* ptr = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(&val));
// 			unsigned size = sizeof(val);
// 			put_buffer(ptr,size);
// 		}
// 		// // std::vector and nested std::vector
// 		// template<typename T>
// 		// void put(const std::vector<T>& std_vector) {
// 		// 	size_t size = std_vector.size();
// 		// 	put(size);
// 		// 	const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_vector[0]));
// 		// 	put_buffer(begin, size);
// 		// }
// 		// template<typename T>
// 		// void put(const std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
// 		// 	size_t size = std_vector_vector.size();
// 		// 	put(size);
// 		// 	for (size_t i = 0; i < size; ++i) {
// 		// 		put(std_vector_vector[i]);
// 		// 	}
// 		// }
// 		// // std::string 
// 		// void put(const std::string& std_string) {
// 		// 	size_t size = std_string.size();
// 		// 	put(size);
// 		// 	const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_string[0]));
// 		// 	put_buffer(begin, size);
// 		// }
// 		// // std::vector<std::string>
// 		// void put(const std::vector<std::string>& vector_string) {
// 		// 	size_t size = vector_string.size();
// 		// 	put(size);
// 		// 	for (size_t i = 0; i < size; ++i) {
// 		// 		put(vector_string[i]);
// 		// 	}
// 		// }
// 		// // std::map
// 		// template<typename K, typename V>
// 		// void put(const std::map<K,V> &std_map) {
// 		// 	size_t size = std_map.size();
// 		// 	put(size);
// 		// 	for (typename std::map<K,V>::const_iterator it = std_map.begin(); it != std_map.end(); ++it) {
// 		// 		put(it->first);
// 		// 		put(it->second);
// 		// 	}
// 		// }


// 		enum WriteResult
// 		{
// 			WRITE_FAILURE = -1,
// 			WRITE_RESCHEDULE = 0,
// 			WRITE_SUCCESS = 1,
// 		};
// 		int write() {
// 			//std::cerr << "write start: ";
// 			std::cerr << "write( " << _fd << " )" << std::endl;
// 			for (int idx = _r_idx; idx != _w_idx; ++idx) {
// 				if (idx == _buffer.size()) {
// 					idx = 0;
// 				}
// 				std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)_buffer[idx] << " ";
// 			}
// 			std::cerr << "  <- content " << std::dec << std::endl;
// 			while (!empty()) {
// 				std::cerr << "  _buffer.size()=" << _buffer.size() << "   _r_idx=" << _r_idx << "   _w_idx=" << _w_idx << std::endl;
// 				if (_r_idx < _w_idx) {
// 					//std::cerr << "r<w" << std::endl;
// 					// write data between _r_idx and _w_idx
// 					//       <---->
// 					// *********************
// 					//      |      |
// 					// _r_idx     _w_idx
// 					unsigned write_size = _w_idx-_r_idx;
// 					write_size = std::min<unsigned>(max_write_size, write_size);
// 					int result = write_fn(_fd, &_buffer[_r_idx], write_size);
// 					if (result < 0) {
// 						//std::cerr << "write failure: _buffer.size()=" << _buffer.size() << "   _r_idx=" << _r_idx << "   _w_idx=" << _w_idx << std::endl;
// 						return write_failure(result);
// 					} 
// 					_r_idx += result;				
// 				} else {
// 					//std::cerr << "w<r" << std::endl;
// 					// write part at the end of the _buffer
// 					//              <------>
// 					// *********************
// 					//      |      |        |
// 					// _w_idx     _r_idx   _buffer.size()
// 					int write_size = std::min<unsigned>(max_write_size, _buffer.size()-_r_idx);
// 					int result = write_fn(_fd, &_buffer[_r_idx], write_size);
// 					if (result <= 0) {
// 						//std::cerr << "write failure: _buffer.size()=" << _buffer.size() << "   _r_idx=" << _r_idx << "   _w_idx=" << _w_idx << std::endl;
// 						return write_failure(result);
// 					} 
// 					_r_idx += result;
// 					if (_r_idx == _buffer.size()) {
// 						_r_idx = 0;
// 					}
// 				}
// 			}
// 			std::cerr << "write success: _buffer.size()=" << _buffer.size() << "   _r_idx=" << _r_idx << "   _w_idx=" << _w_idx << std::endl;
// 			return WRITE_SUCCESS;
// 		}


// 		void put_buffer(const unsigned char* ptr, unsigned size) {
// 			std::cerr << "put buffer" << size << std::endl;
// 			for (int idx = 0; idx < size; ++idx) {
// 				std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[idx] << " ";
// 			}
// 			std::cerr << "  <- add this" << std::dec << std::endl;

// 			while (size > unused()) {
// 				std::cerr << " _r_idx=" << _r_idx << "  _w_idx=" << _w_idx << std::endl;
// 				std::cerr << "resizing ";
// 				unsigned old_buffer_size = _buffer.size();
// 				_buffer.resize(2*_buffer.size());
// 				if (_w_idx < _r_idx) {
// 					memcpy(&_buffer[old_buffer_size], &_buffer[0], _w_idx);
// 					_w_idx += old_buffer_size;
// 				}
// 				std::cerr << " _r_idx=" << _r_idx << "  _w_idx=" << _w_idx << std::endl;
// 			}
// 			// from this point there is enough unused space and data can be put into _buffer
// 			// the only special case to handle is when _w_idx is too close to _buffer's end
// 			if (_w_idx+size >= _buffer.size()) {
// 				unsigned first_half = _buffer.size() - _w_idx;
// 				unsigned second_half = size - first_half;
// 				memcpy(&_buffer[_w_idx], ptr, first_half);
// 				memcpy(&_buffer[0], ptr+first_half, second_half);
// 				_w_idx = second_half;
// 			} else {
// 				memcpy(&_buffer[_w_idx], ptr, size);
// 				_w_idx += size;
// 			}
// 			for (int idx = _r_idx; idx != _w_idx; ++idx) {
// 				if (idx == _buffer.size()) {
// 					idx = 0;
// 				}
// 				std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)_buffer[idx] << " ";
// 			}
// 			std::cerr << "  <- content " << std::dec << std::endl;
// 		}

// 	private:
// 		// to be called when write_fn returns -1
// 		// evaluates errno to decide how to handle it
// 		// returns WRITE_RESCHEDULE if there is still data (errno == EAGAIN or EWOULDBLOCK)
// 		// returns WRITE_FAILURE if the socket is broken, i.e. peer hung up (TODO: not implemented yet)
// 		// throws if the error is unknown/not handled explicitely
// 		int write_failure(int write_result) {
// 			if (errno == EAGAIN || errno == EWOULDBLOCK) {
// 				return WRITE_RESCHEDULE;
// 			} else {
// 				std::ostringstream msg;
// 				msg << "WriteBuffer::write() unknown errno: " << errno << " = " << strerror(errno) << std::endl;
// 				throw std::runtime_error(msg.str());
// 			}
// 		}

// 		int _fd; // file descriptor
// 		unsigned _w_idx, _r_idx;
// 		const unsigned max_write_size = 16384;
// 		std::vector<unsigned char> _buffer;
// 	};


// 	ssize_t write_write(int fd, const void *buf, size_t count);
// 	// {
// 	// 	for (unsigned i = 0; i < count; ++i) {
// 	// 		std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)*(unsigned char*)buf << " " << std::dec;
// 	// 	}
// 	// 	return ::write(fd, buf, count);
// 	// }	

// 	typedef WriteBuffer<write_write> SerialBuffer;

}

#endif
