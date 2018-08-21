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
		if (_debug_level > 5) std::cerr << "scalar write: " << scalar << std::endl;
		int result = write_all(fd, static_cast<const void*>(&scalar), sizeof(scalar));
		if (_debug_level > 5) std::cerr << "done " << std::endl;
		return result;
	}
	template<typename T>
	int read(int fd, T & scalar) {
		if (_debug_level > 5) std::cerr << "scalar read" << std::endl;
		int result = read_all(fd, static_cast<void*>(&scalar), sizeof(scalar));
		if (_debug_level > 5) std::cerr << scalar << "  done " << std::endl;
		return result;
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
	// specialize for Glib::ustring and std::string
	template<>
	int write<Glib::ustring>(int fd, const Glib::ustring & std_vector);
	template<>
	int read<Glib::ustring>(int fd, Glib::ustring & std_vector);
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
}

#endif
