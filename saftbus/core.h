#ifndef SAFTBUS_CORE_H_
#define SAFTBUS_CORE_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <giomm.h>

namespace saftbus 
{


	extern int _debug_level;

	void init();

	int write_all(int fd, const void *buffer, int size);
	int read_all(int fd, void *buffer, int size);

	int sendfd(int socket, int fd);
	int recvfd(int socket);

	template<typename T>
	int write(int fd, const T & scalar)	{
		//if (_debug_level > 5) std::cerr << "scalar write: " << scalar << std::endl;
		int result = write_all(fd, static_cast<const void*>(&scalar), sizeof(scalar));
		//if (_debug_level > 5) std::cerr << "done " << std::endl;
		return result;
	}
	template<typename T>
	int read(int fd, T & scalar) {
		//if (_debug_level > 5) std::cerr << "scalar read" << std::endl;
		int result = read_all(fd, static_cast<void*>(&scalar), sizeof(scalar));
		//if (_debug_level > 5) std::cerr << scalar << "  done " << std::endl;
		return result;
	}

	// std::set (not nested) 
	template<typename T>
	int write(int fd, const std::set<T>& std_set) {
		guint32 size = std_set.size();
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
		guint32 size;
		int result = read(fd, size);
		if (result == -1) return result;
		std_set.clear();
		for (guint32 i = 0; i < size; ++i) {
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
		guint32 size = std_vector.size();
		int result = write_all(fd, static_cast<void*>(&size), sizeof(guint32));
		if (result == -1) return result;
		if (size > 0) return write_all(fd, static_cast<const void*>(&std_vector[0]), size*sizeof(decltype(std_vector.back())));
	}
	template<typename T>
	int write(int fd, const std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
		if (_debug_level > 5) std::cerr << "nested vector write" << std::endl;
		guint32 size = std_vector_vector.size();
		int result = write_all(fd, static_cast<const void*>(&size), sizeof(guint32));
		if (result == -1) return result;
		for (guint32 i = 0; i < size; ++i) {
			result = write(fd, std_vector_vector[i]);
			if (result == -1) return result;
		}
		return 1; // meaning success
	}
	template<typename T>
	int read(int fd, std::vector<T> & std_vector) {
		if (_debug_level > 5) std::cerr << "vector read" << std::endl;
		guint32 size;
		int result = read_all(fd, static_cast<void*>(&size), sizeof(guint32));
		if (result == -1) return result;
		std_vector.resize(size);
		if (size > 0) return read_all(fd, static_cast<void*>(&std_vector[0]), size*sizeof(decltype(std_vector.back())));
	}
	template<typename T>
	int read(int fd, std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
		if (_debug_level > 5) std::cerr << "nested vector read" << std::endl;
		guint32 size;
		int result = read_all(fd, static_cast<void*>(&size), sizeof(guint32));
		if (result == -1) return result;
		std_vector_vector.resize(size);
		for (guint32 i = 0; i < size; ++i) {
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
		guint32 size = map.size();
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
		guint32 size;
		int result = read(fd, size);
		if (result == -1) return result;
		K k;  V v;
		for (guint32 i = 0; i < size; ++i) {
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


	bool deserialize(Glib::Variant<std::vector<Glib::VariantBase> > &result, const char *data, gsize size);









	class Serial
	{
	public:
		Serial()
		{
			_iter = _data.begin();
		}
		// hast to be called before any call to get()
		void get_init(const std::vector<char> &data) 
		{
			_data = data;
			get_init();
		}
		// hast to be called before any call to get()
		void get_init() 
		{
			_iter = _data.begin();
		}
		// POD structs and build-in types
		template<typename T>
		void put(const T &val) {
			char* begin = const_cast<char*>(reinterpret_cast<const char*>(&val));
			char* end   = begin + sizeof(val);
			_data.insert(_data.end(), begin, end);
		}
		template<typename T>
		void get(T &val) {
			val    = *reinterpret_cast<T*>(&(*_iter));
			_iter += sizeof(val);
		}
		// std::vector and nested std::vector
		template<typename T>
		void put(const std::vector<T>& std_vector) {
			size_t size = std_vector.size();
			put(size);
			char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_vector[0]));
			char* end   = begin + size*sizeof(std_vector[0]);
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
		void get(std::vector<T> &std_vector) {
			size_t size;
			get(size);
			T* begin = reinterpret_cast<T*>(&(*_iter));
			T* end   = begin + size;
			std_vector.clear();
			std_vector.insert(std_vector.end(), begin, end);
			_iter += sizeof(T)*size;
		}
		template<typename T>
		void get(std::vector< std::vector<T, std::allocator<T> >, std::allocator< std::vector<T, std::allocator<T> > > >& std_vector_vector) {
			size_t size;
			get(size);
			std_vector_vector.resize(size);
			for (size_t i = 0; i < size; ++i) {
				get(std_vector_vector[i]);
			}
		}
		// std::string and std::vector of std::string
		void put(const std::string& std_string) {
			size_t size = std_string.size();
			put(size);
			char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_string[0]));
			char* end   = begin + size*sizeof(std_string[0]);
			_data.insert(_data.end(), begin, end);
		}
		void get(std::string &std_string) {
			size_t size;
			get(size);
			char* begin = &(*_iter);
			char* end   = begin + size;
			std_string.clear();
			std_string.insert(std_string.end(), begin, end);
			_iter += size;
		}
		// nested Serials
		void put(const Serial &ser) {
			put(ser._data);
		}
		void get(Serial &ser) {
			get(ser._data);
		}

		std::vector<char>& data() {
			return _data;
		}
		void print() {
			std::cerr << _data.size() << std::endl;
			for (auto ch: _data) {
				std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)ch << std::dec << std::endl;
			}
		}
	private:
		std::vector<char> _data;
		std::vector<char>::iterator _iter;
	};



}

#endif
