cmake_minimum_required(VERSION 3.21...4.0)

set(MAJOR "3")      # Increment if removed/changed properties/signals/methods since previous release
set(MINOR "2")      # Increment if added properties/signals/methods; reset to 0 if MAJOR changed
set(REVISION "3")   # Increment on each release; reset to 0 if MAJOR/MINOR changed
set(SONAME "10")    # Whenever MAJOR is incremented, add MINOR+1 to this variable

# Project setup
project(saftlib
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    DESCRIPTION "Simplified API for Timing"
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

include(${CMAKE_SOURCE_DIR}/cmake/generator.cmake)

# ============================================================================
# Sources to be generated
# ============================================================================

set (GENERATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BuildIdRom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BurstGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Condition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA_Event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA_TLU.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmbeddedCPUActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmbeddedCPUCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EventSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorFirmware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LM32Cluster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MasterFunctionGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OpenDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Output.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OutputCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Owned.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Reset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SAFTd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SCUbusActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SCUbusCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SoftwareActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SoftwareCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TempSensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TimingReceiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WbmActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WbmCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WhiteRabbit.cpp
)

add_custom_target( generate_sources )
generate_saftbus_files( "${GENERATOR_SOURCES}" generate_sources )

# ============================================================================
# Main Library: saftlib
# ============================================================================

# Source files
set(LIBSAFT_SERVICE_SOURCES
    #${CMAKE_CURRENT_SOURCE_DIR}/src/build.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/eb-source.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/eb-forward.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Owned.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SAFTd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Condition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SoftwareCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SoftwareActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SCUbusCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SCUbusActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmbeddedCPUCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmbeddedCPUActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WbmCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WbmActionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SdbDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MsiDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OpenDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WhiteRabbit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Mailbox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Watchdog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SerdesClockGen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Io.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IoControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Output.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OutputCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EventSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA_TLU.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECA_Event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BuildIdRom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TempSensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Reset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LM32Cluster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TimingReceiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TimingReceiverAddon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CommonFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/create_service.cpp
    ${SAFTLIB_GENERATED_FILES}
)

# Create the shared library
add_library(saft_service SHARED ${LIBSAFT_SERVICE_SOURCES})

# Set library properties
set_target_properties(saft_service PROPERTIES
    VERSION @SAFTD_API@.@SAFTD_REVISION@.@SAFTD_MINOR@
    SOVERSION @SAFTD_API@
    LINK_FLAGS "-Wl,--export-dynamic"
)

target_include_directories(saft_service
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(saft_service
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        dl
)

