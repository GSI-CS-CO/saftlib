#ifndef SAFTBUS_SAFTBUS_HPP_
#define SAFTBUS_SAFTBUS_HPP_

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>

namespace saftbus {

	int write_all(int fd, const char *buffer, int size);
	int read_all(int fd, char *buffer, int size);

	// send a file descriptor 
	int sendfd(int socket, int fd);
	// receive a file descriptor
	int recvfd(int socket);


	// Simple classes for serialization and de-serialization 
	// without storing type information, i.e. de-serialization 
	// only works if the type composition is known (but this 
	// is the case in all saftlib transfers)
	// sending data works like this:
	//   serializer.put(value1);
	//   serializer.put(value2);
	//   ...
	//   serializer.write_to(fd);
	//
	// receiving data works like this:
	//   deserializer.read_from(fd);
	//   deserializer.get(value1);
	//   deserializer.get(value2);
	//   ...

	class Serializer
	{
	public:
		Serializer(int reserve = 4096)
		{
			_data.reserve(reserve);
			_iter = _data.begin();
		}

		// write the length of the serdes data buffer and the buffer content to file descriptor fd
		bool write_to(int fd);
		bool write_to_no_init(int fd);

		// POD struct and build-in types
		template<typename T>
		void put(const T &val) {
			while(_data.size()%sizeof(T) != 0) _data.push_back('x'); // insert padding (reading from address that is not aligned to target type is undefined behavior)
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&val));
			const char* end   = begin + sizeof(val);
			_data.insert(_data.end(), begin, end);
		}
		// std::vector and nested std::vector
		template<typename T>
		void put(const std::vector<T>& std_vector) {
			size_t size = std_vector.size();
			put(size);
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_vector[0]));
			const char* end   = begin + size*sizeof(std_vector[0]);
			while(_data.size()%sizeof(T) != 0) _data.push_back('x'); // insert padding (reading from address that is not aligned to target type is undefined behavior)
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
		// std::string 
		void put(const std::string& std_string) {
			size_t size = std_string.size();
			put(size);
			const char* begin = const_cast<char*>(reinterpret_cast<const char*>(&std_string[0]));
			const char* end   = begin + size*sizeof(std_string[0]);
			_data.insert(_data.end(), begin, end);
		}
		// std::vector<std::string>
		void put(const std::vector<std::string>& vector_string) {
			size_t size = vector_string.size();
			put(size);
			for (size_t i = 0; i < size; ++i) {
				put(vector_string[i]);
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
		// // nested Serializer
		// void put(Serializer &ser) {
		// 	put(ser._data);
		// 	ser.put_init();
		// }

		bool empty();

		// has to be called before first call to put()
		void put_init();
	private:


		std::vector<char> _data;
		mutable std::vector<char>::const_iterator _iter;
	};


	class Deserializer
	{
	public:
		Deserializer(int reserve = 4096)
		{
			_data.reserve(reserve);
			_iter = _data.begin();
		}

		// fill the serdes data buffer by reading data from the file descriptor fd
		bool read_from(int fd);

		// POD struct and build-in types
		template<typename T>
		void get(T &val) const {
			while((_iter-_data.begin())%sizeof(T) != 0) _iter+=sizeof('x'); // insert padding (reading from address that is not aligned to target type is undefined behavior)
			val    = *const_cast<T*>(reinterpret_cast<const T*>(&(*_iter)));
			_iter += sizeof(val);
		}
		// std::vector and nested std::vector
		template<typename T>
		void get(std::vector<T> &std_vector) const {
			size_t size;
			get(size);
			while((_iter-_data.begin())%sizeof(T) != 0) _iter+=sizeof('x'); // insert padding (reading from address that is not aligned to target type is undefined behavior)
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
		void get(std::string &std_string) const {
			size_t size;
			get(size);
			const char* begin = &(*_iter);
			const char* end   = begin + size;
			std_string.clear();
			std_string.insert(std_string.end(), begin, end);
			_iter += size;
		}
		// std::vector<std::string>
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
		// // nested Deserializer
		// void get(Deserializer &ser) const {
		// 	get(ser._data);
		// 	ser.get_init();
		// }

		void save() const;
		void restore() const;

	private:

		// has to be called before first call to get()
		void get_init() const;

		std::vector<char> _data;
		mutable std::vector<char>::const_iterator _iter;
		mutable std::vector<char>::const_iterator _saved_iter;
	};




}

#endif
