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

find_package(PkgConfig REQUIRED)
pkg_check_modules(EB REQUIRED etherbone>=2.1.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(LIBRTPI REQUIRED librtpi>=1.0.1)

# Add these definitions globally
add_definitions(
    -DPACKAGE_STRING="${PROJECT_NAME} ${PROJECT_VERSION}"
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
add_definitions(
    -DUSERNAME="${USERNAME}"
    -DHOSTNAME="${HOSTNAME}"
    -DOPERATING_SYSTEM="${OPERATING_SYSTEM}"
)

add_library(saftbus SHARED
    ./src/loop.cpp
    ./src/saftbus.cpp
    ./src/error.cpp
    ./src/client.cpp
    ./src/service.cpp
    ./src/plugins.cpp
    ./src/server.cpp
    ./include/chunck_allocator_rt.hpp
    ./include/client.hpp
    ./include/configurable_chunck_allocator_rt.hpp
    ./include/error.hpp
    ./include/global_allocator.hpp
    ./include/loop.hpp
    ./include/plugins.hpp
    ./include/saftbus.hpp
    ./include/server.hpp
    ./include/service.hpp
)

target_include_directories(saftbus PUBLIC
    ${saftbus_SOURCE_DIR}/include
)

target_compile_options(saftbus PRIVATE
    -fPIC
    ${SIGCPP_CFLAGS}
    ${LIBRTPI_CFLAGS}
)

set_target_properties(saftbus PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${SONAME}
)

add_executable(saftbusd)
target_sources(saftbusd
    PRIVATE
        ./src/saftbusd.cpp
        ./src/configurable_chunck_allocator_rt.cpp
)
target_link_libraries(saftbusd
    ${SIGCPP_LIBRARIES}
    ${LIBRTPI_LIBRARIES}
    saftbus
    dl
    pthread
)
target_compile_options(saftbusd PRIVATE
    ${LIBRTPI_CFLAGS}
)
target_compile_definitions(saftbusd
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)


add_executable(saftbusd_sda)
target_sources(saftbusd_sda
    PRIVATE
        ./src/saftbusd-sda.cpp
        ./src/print_allocator_fillstate.cpp
)
target_link_libraries(saftbusd_sda
    ${SIGCPP_LIBRARIES}
    ${LIBRTPI_LIBRARIES}
    saftbus
    dl
    pthread
)
target_compile_definitions(saftbusd_sda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)

add_executable(saftbusd_noda)
target_sources(saftbusd_noda
    PRIVATE
        ./src/saftbusd-noda.cpp
        ./src/print_allocator_fillstate.cpp
)
target_link_libraries(saftbusd_noda
    ${SIGCPP_LIBRARIES}
    ${LIBRTPI_LIBRARIES}
    saftbus
    dl
    pthread
)
target_compile_definitions(saftbusd_noda
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)


add_executable(saftbus_ctl)
target_sources(saftbus_ctl
    PRIVATE
        ./tools/saftbus-ctl.cpp
)
target_link_libraries(saftbus_ctl
    ${SIGCPP_LIBRARIES}
    ${LIBRTPI_LIBRARIES}
    saftbus
    dl
    pthread
)
target_compile_definitions(saftbus_ctl
    PRIVATE
        VERSION="${PROJECT_VERSION}"
)


# Create pkg-config file
configure_file(
    ${saftbus_SOURCE_DIR}/saftbus.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/saftbus.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkg
)

# Install the library
install(TARGETS saftbus
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install header files
install(FILES
    ./include/chunck_allocator_rt.hpp
    ./include/client.hpp
    ./include/configurable_chunck_allocator_rt.hpp
    ./include/error.hpp
    ./include/global_allocator.hpp
    ./include/loop.hpp
    ./include/plugins.hpp
    ./include/saftbus.hpp
    ./include/server.hpp
    ./include/service.hpp
    DESTINATION include/saftbus
)

# Install executables
install(TARGETS
    saftbusd
    saftbusd_sda
    saftbusd_noda
    saftbus_ctl
    RUNTIME DESTINATION bin
)