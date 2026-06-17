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

# ============================================================================
# Env Foo
# ===========================================================================

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/environment.cmake)
# Get username, hostname, and operating system
get_env()

# MESSAGE (STATUS "GIT_ID: ${GIT_ID}")
# MESSAGE (STATUS "SOURCE_DATE: ${SOURCE_DATE}")

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
)


# ============================================================================
# Main Library: saftbus
# ============================================================================

add_library(saftbus SHARED)

target_sources(saftbus

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/loop.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbus.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp

    PUBLIC
    FILE_SET saftbus_public_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/chunck_allocator_rt.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/client.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/error.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/global_allocator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/loop.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/plugins.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/saftbus.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/server.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service.hpp

    PRIVATE
    FILE_SET saftbus_private_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES
        # here because we do not want it to be installed
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configurable_chunck_allocator_rt.hpp

)

# Should be put into target_sources above
target_include_directories(saftbus
    PUBLIC
        ${SIGCPP_INCLUDE_DIRS}
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


# ============================================================================
# Executable: saftbusd
# ============================================================================

add_executable(saftbusd)

target_sources(saftbusd

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configurable_chunck_allocator_rt.cpp
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

add_executable(saftbusd-sda)

target_sources(saftbusd-sda

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-sda.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
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

add_executable(saftbusd-noda)

target_sources(saftbusd-noda

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-noda.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
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

add_executable(saftbus-ctl)

target_sources(saftbus-ctl

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/saftbus-ctl.cpp
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

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    saftbus.cmake
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Prepare the pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/saftbus.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    @ONLY
)

# Prepare the saftbus-service file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/saftbus.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.service
    @ONLY
)


# ============================================================================
# Installation
# ============================================================================

install(
    TARGETS
        saftbus
        saftbusd
        saftbusd-sda
        saftbusd-noda
        saftbus-ctl

    FILE_SET saftbus_public_headers
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# The saftbus.service file is generated but not installed. The service file
# is usually only used on Yocto images. The corresponding recipe needs to deal
# with its installation
