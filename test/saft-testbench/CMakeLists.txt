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
# Saft Testbench
# ===========================================================================

add_executable( saft-testbench
  saft-testbench.cpp
)

target_link_libraries( saft-testbench
    PRIVATE
        saft-proxy
        dl
)


# ============================================================================
# Installation
# ===========================================================================

install(TARGETS
    saft-testbench
)
