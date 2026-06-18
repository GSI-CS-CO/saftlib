cmake_minimum_required(VERSION 3.23...4.0)

set(MAJOR "1")      # Increment if removed/changed properties/signals/methods since previous release
set(MINOR "0")      # Increment if added properties/signals/methods; reset to 0 if MAJOR changed
set(REVISION "0")   # Increment on each release; reset to 0 if MAJOR/MINOR changed
set(SONAME "1")    # Whenever MAJOR is incremented, add MINOR+1 to this variable

# Project setup
project(saft-tools
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    DESCRIPTION "CLI tools for saftbus"
    LANGUAGES C CXX
)


# Set C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# Use standard CMake install directories
include(GNUInstallDirs)

# ============================================================================
# Software Timing Receiver
# ===========================================================================

add_executable( saft-software-tr
  saft-software-tr.cpp
)

target_compile_definitions(saft-software-tr PRIVATE
    DATADIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}"
)

target_link_libraries( saft-software-tr
    PRIVATE
        saft-service
)


# ============================================================================
# Installation
# ===========================================================================

install(TARGETS
    saft-software-tr
)

install(FILES software-tr.sdb
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saftlib/)
