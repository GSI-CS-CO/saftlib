#include "core.h"
#include <stdexcept>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <sys/file.h> // for flock()

namespace saftbus
{
  int _debug_level = 1;

  int write_all(int fd, const void *buffer, int size)
  {
    //std::cerr << "write_all(buffer, " << size << ") called " << std::endl;
    const char *ptr = static_cast<const char*>(buffer);
    int n_written = 0;
    do {
      int result = ::write(fd, ptr, size-n_written);
      //std::cerr << "write result = " << result << " " << std::endl;
      if (result > 0)
      {
        ptr       += result;
        n_written += result;
      }
      else
      {
        if (result == 0) throw std::runtime_error(" could not write to pipe");
        else             throw std::runtime_error(strerror(errno));
      }
    } while (n_written < size);
    return n_written;
  }

  int read_all(int fd, void *buffer, int size)
  {
    //std::cerr << "read_all(buffer, " << size << ") called " << std::endl;
    char* ptr = static_cast<char*>(buffer);
    int n_read = 0;
    do { 
      int result = ::read(fd, ptr, size-n_read);//, MSG_DONTWAIT); 
      //std::cerr << "read result = " << result << " " << std::endl;
      if (result > 0)
      {
        ptr    += result;
        n_read += result;
      } else {
        if (result == 0) {
          if (n_read) {// if we already got some of the data, we try again
            std::cerr << "n_read = " << n_read << " ... that is why we continue ( size = " << size << " ) " << std::endl;
            continue;;
          } else {
            return -1; // -1 means end of file
          }
        }
        else             throw std::runtime_error(strerror(errno));
      }
    } while (n_read < size);
    return n_read;
  }


  template<>
  int write<Glib::ustring>(int fd, const Glib::ustring & std_vector) {
    if (_debug_level) std::cerr << "ustring write \"" << std_vector << "\"" << std::endl;
    std::string strg(std_vector);
    guint32 size = strg.size();
    int result = write_all(fd, static_cast<const void*>(&size), sizeof(guint32));
    if (_debug_level) std::cerr << "ustring write, result = " << result << std::endl;
    if (result == -1) return result;
    if (size > 0) result =  write_all(fd, static_cast<const void*>(&strg[0]), size*sizeof(decltype(strg.back())));
    if (_debug_level) std::cerr << "ustring write, result = " << result << std::endl;
    return result;
  }
  template<>
  int read<Glib::ustring>(int fd, Glib::ustring & std_vector) {
    if (_debug_level) std::cerr << "ustring read" << std::endl;
    std::string strg(std_vector);
    guint32 size;
    int result = read_all(fd, static_cast<void*>(&size), sizeof(guint32));
    if (result == -1) return result;
    strg.resize(size);
    if (size > 0) result = read_all(fd, static_cast<void*>(&strg[0]), size*sizeof(decltype(strg.back())));
    std_vector = strg;
    return result;
  }

  template<>
  int write<std::string>(int fd, const std::string & std_vector) {
    if (_debug_level) std::cerr << "std::string write " << std_vector << std::endl;
    std::string strg(std_vector);
    guint32 size = strg.size();
    int result = write_all(fd, static_cast<const void*>(&size), sizeof(guint32));
    if (result == -1) return result;
    if (size > 0) return write_all(fd, static_cast<const void*>(&strg[0]), size*sizeof(decltype(strg.back())));
    return 1;
  }
  template<>
  int read<std::string>(int fd, std::string & std_vector) {
    if (_debug_level) std::cerr << "std::string read" << std::endl;
    std::string strg(std_vector);
    guint32 size;
    int result = read_all(fd, static_cast<void*>(&size), sizeof(guint32));
    if (result == -1) return result;
    strg.resize(size);
    if (size > 0) result = read_all(fd, static_cast<void*>(&strg[0]), size*sizeof(decltype(strg.back())));
    std_vector = strg;
    return result;
  }





}
