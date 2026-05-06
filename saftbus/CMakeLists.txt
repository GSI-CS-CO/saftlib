cmake_minimum_required(VERSION 3.21...4.0)

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

# Get username, hostname, and operating system
if(UNIX)
    # Use environment variables or system commands to get these
    execute_process(
        COMMAND whoami
        OUTPUT_VARIABLE USERNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND uname -s
        OUTPUT_VARIABLE OPERATING_SYSTEM
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Fallback for non-Unix systems
    set(USERNAME "$ENV{USERNAME}")
    set(HOSTNAME "$ENV{COMPUTERNAME}")
    set(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")
endif()

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
    PRIVATE
        ${EB_INCLUDE_DIRS}
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbus
    PRIVATE
        ${EB_LIBRARIES}
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
)

set_target_properties(saftbus PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${SONAME}
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

# ============================================================================
# Executable: saftbusd_sda
# ============================================================================

add_executable(saftbusd_sda
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-sda.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
)

target_include_directories(saftbusd_sda
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbusd_sda
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbusd_sda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

# ============================================================================
# Executable: saftbusd_noda
# ============================================================================

add_executable(saftbusd_noda
    ${CMAKE_CURRENT_SOURCE_DIR}/src/saftbusd-noda.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/print_allocator_fillstate.cpp
)

target_include_directories(saftbusd_noda
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbusd_noda
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbusd_noda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

# ============================================================================
# Executable: saftbus_ctl
# ============================================================================

add_executable(saftbus_ctl
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/saftbus-ctl.cpp
)

target_include_directories(saftbus_ctl
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saftbus_ctl
    PRIVATE
        saftbus
        ${SIGCPP_LIBRARIES}
        ${LIBRTPI_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
)

target_compile_definitions(saftbus_ctl
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

# Install library
install(TARGETS saftbus
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftbus
)

# Install header files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/chunck_allocator_rt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/client.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/configurable_chunck_allocator_rt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/global_allocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/plugins.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/saftbus.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/service.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftbus
)

# Install executables
install(TARGETS
    saftbusd
    saftbusd_sda
    saftbusd_noda
    saftbus_ctl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install pkg-config file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
