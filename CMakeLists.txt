cmake_minimum_required(VERSION 3.23...4.0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()


# Set the project name and language
project(
  saft
  VERSION 3.3.1
  DESCRIPTION "Simplified API for Timing"
  LANGUAGES CXX C)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(saftbus)
add_subdirectory(saftbus-gen)
add_subdirectory(saftlib)
add_subdirectory(tools)