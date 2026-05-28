cmake_minimum_required(VERSION 3.23...4.0)

set(MAJOR "3")      # Increment if removed/changed properties/signals/methods since previous release
set(MINOR "2")      # Increment if added properties/signals/methods; reset to 0 if MAJOR changed
set(REVISION "3")   # Increment on each release; reset to 0 if MAJOR/MINOR changed
set(SONAME "10")    # Whenever MAJOR is incremented, add MINOR+1 to this variable

# Project setup
project(saftbus
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    DESCRIPTION "An interprocess communication library"
    LANGUAGES C CXX
)

# Set C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# Use standard CMake install directories
include(GNUInstallDirs)
# Enable easy creation of package config files
include(CMakePackageConfigHelpers)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(EB REQUIRED etherbone>=2.1.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(LIBRTPI REQUIRED librtpi>=1.0.1)

# Add global preprocessor definitions
add_compile_definitions(
    PACKAGE_STRING="${PROJECT_NAME} ${PROJECT_VERSION}"
)

include(${CMAKE_SOURCE_DIR}/cmake/environment.cmake)
# Get username, hostname, and operating system
get_env()
# Add definitions for these variables
add_compile_definitions(
    USERNAME="${USERNAME}"
    HOSTNAME="${HOSTNAME}"
    OPERATING_SYSTEM="${OPERATING_SYSTEM}"
)

# ============================================================================
# Main Library: saftbus
# ============================================================================

add_library(saftbus SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
)

target_include_directories(saftbus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/saftbus>
        ${EB_INCLUDE_DIRS}
        ${SIGCPP_INCLUDE_DIRS}
    PRIVATE
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbus
    PUBLIC
        ${EB_LIBRARIES}
        ${SIGCPP_LIBRARIES}
    PRIVATE
        ${LIBRTPI_LIBRARIES}
)

set_target_properties(saftbus PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${SONAME}
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/chunck_allocator_rt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/client.hpp
    # commented out because was not installed in autotools project
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/configurable_chunck_allocator_rt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/global_allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/plugins.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/saftbus.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/service.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ============================================================================
# Executable: saftbusd
# ============================================================================

add_executable(saftbusd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/configurable_chunck_allocator_rt.cpp
)

target_include_directories(saftbusd
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbusd
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbusd
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

install(TARGETS saftbusd)


# ============================================================================
# Executable: saftbusd-sda
# ============================================================================

add_executable(saftbusd-sda
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-sda.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
)

target_include_directories(saftbusd-sda
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbusd-sda
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbusd-sda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

# ============================================================================
# Executable: saftbusd-noda
# ============================================================================

add_executable(saftbusd-noda
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-noda.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
)

target_include_directories(saftbusd-noda
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbusd-noda
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbusd-noda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

# ============================================================================
# Executable: saftbus-ctl
# ============================================================================

add_executable(saftbus-ctl
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/saftbus-ctl.cpp
)

target_include_directories(saftbus-ctl
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbus-ctl
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbus-ctl
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

# ============================================================================
# Package Configuration
# ============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/saftbus.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    @ONLY
)

# ============================================================================
# Installation
# ============================================================================

# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     MyLibConfigVersion.cmake
#     VERSION ${PACKAGE_VERSION}
#     COMPATIBILITY AnyNewerVersion
#     )

install(
    TARGETS saftbus
)

#  /usr/bin/mkdir -p '/usr/local/lib'
#  /bin/bash ./libtool   --mode=install /usr/bin/install -c
#  libsaftbus.la
#  libsaft-service.la
#  libsaft-proxy.la
#  libfg-firmware-service.la
#  libfg-firmware-proxy.la
#  libbg-firmware-service.la
#  libbg-firmware-proxy.la
#  '/usr/local/lib'
# libtool: install: /usr/bin/install -c
# .libs/libsaftbus.so.10.3.1
# /usr/local/lib/libsaftbus.so.10.3.1



# # Install library
# install(TARGETS saftbus
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftbus
# )

# # Install header files
# install(FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/chunck_allocator_rt.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/client.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/configurable_chunck_allocator_rt.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/error.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/global_allocator.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/loop.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/plugins.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/saftbus.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/server.hpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/service.hpp
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftbus
# )

