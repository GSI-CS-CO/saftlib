/*  Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#ifndef SAFTBUS_SAFTBUS_HPP_
#define SAFTBUS_SAFTBUS_HPP_

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>

/// @brief classes and functions of the saftbus interprocess communication library.
/// 
/// * Client side classes are in client.hpp and client.cpp
///   * ClientConnection class
///   * Proxy base class
///   * Container_Proxy class
///   * SignalGroup class
/// * Server side classes are in server.hpp, server.cpp and service.hpp, service.cpp
///   * ServerConnection class
///   * Service base class
///   * Container class
///   * Container_Service class
/// * Data (de-)serialization happens in saftbus.hpp, saftbus.cpp
/// * PluginLoader is in plugins.hpp, plugins.cpp
/// * saftbus and provides an event loop implementation and two event sources in loop.hpp, loop.cpp
///   * Loop class
///   * Source base class
///   * TimeoutSource class
///   * IoSource class
namespace saftbus {

	enum class FunctionResult {
		RETURN,
		EXCEPTION,
	};

	int write_all(int fd, const char *buffer, int size);
	int read_all(int fd, char *buffer, int size);

	// send a file descriptor 
	int sendfd(int socket, int fd);
	// receive a file descriptor
	int recvfd(int socket);

	class Serializer;
	class Deserializer;

	/// @brief custom types can be sent over saftbus if they derive from 
	/// this class and implement serialize and deserializ methods
	struct SerDesAble {
		virtual ~SerDesAble() = default;
		virtual void serialize(Serializer &ser) const = 0;
		virtual void deserialize(const Deserializer &des) = 0;
	};


	// Simple classes for serialization and de-serialization 
	// without storing type information, i.e. de-serialization 
	// only works if the type composition is known (but this 
	// is the case in all saftbus transfers)
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

		// this looses in overload resolution against put<SerDesAble>(const T &val)
		// so the wrong function is called... :(
		// void put(const SerDesAble &val) {
		// 	std::cerr << "put(SerDesAble)" << std::endl;
		// 	val.serialize(*this);
		// }

    // template <typename T>
    // std::enable_if_t<!std::is_base_of<serializable,T>::value> put(const T&) { std::cout << "const T& \n";}

    // template <typename T>
    // typename std::enable_if<std::is_base_of<serializable,T>::value>::type put(const T&) { std::cout << "T inherits from serializable \n";}


		// Types derived from SerDesAble 
		template<typename T>
		typename std::enable_if<std::is_base_of<SerDesAble,T>::value>::type put(const T &val) {
			val.serialize(*this);
		}

		// POD struct and build-in types
		template<typename T>
		typename std::enable_if<!std::is_base_of<SerDesAble,T>::value>::type put(const T &val) {
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

		// Types derived from SerDesAble
		template<typename T>
		typename std::enable_if<std::is_base_of<SerDesAble,T>::value>::type // this method competed in overload resulution with template<typename T> get(T &val). "enable_if" lets this version win if a daughter class of SerDesAble is used.
		get(T &val) const {
			val.deserialize(*this);
		}

		// POD struct and build-in types
		template<typename T>
		typename std::enable_if<!std::is_base_of<SerDesAble,T>::value>::type // "enable_if" excludes this method from the overload resolution for all tpes derived from SerDesAble.
		get(T &val) const {
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
		void get(std::map<K,V> &std_map) const {
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
