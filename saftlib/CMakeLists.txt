cmake_minimum_required(VERSION 3.23...4.0)

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
# Enable easy creation of package config files
include(CMakePackageConfigHelpers)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(EB REQUIRED etherbone>=2.1.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(LIBRTPI REQUIRED librtpi>=1.0.1)


# ============================================================================
# Env Foo
# ===========================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/environment.cmake)
# Get username, hostname, and operating system
get_env()

# MESSAGE (STATUS "GIT_ID: ${GIT_ID}")
# MESSAGE (STATUS "SOURCE_DATE: ${SOURCE_DATE}")
# MESSAGE (STATUS "USERNAME: ${USERNAME}")
# MESSAGE (STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
# MESSAGE (STATUS "saftlib_VERSION: ${saftlib_VERSION}")
# MESSAGE (STATUS "PACKAGE_STRING: ${PROJECT_NAME} ${PROJECT_VERSION}")

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
)

# ============================================================================
# Saftbus generator mechanics
# ============================================================================

# get generator function
include(cmake/generator.cmake)

# set where the generates sources go
set(SAFTLIB_GENERATOR_DIR ${saftlib_BINARY_DIR}/generated)

# set ssources to be used as generator input
set (GENERATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ActionSink.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BuildIdRom.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BurstGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Condition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_Event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_TLU.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUCondition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUActionSink.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/EventSource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGeneratorFirmware.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Input.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/LM32Cluster.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MasterFunctionGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/OpenDevice.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Output.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/OutputCondition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Owned.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Reset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SAFTd.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusActionSink.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusCondition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareActionSink.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareCondition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TempSensor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TimingReceiver.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmActionSink.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmCondition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WhiteRabbit.hpp
)

# Create a custom target to ensure generation of all service files
add_custom_target(all_generated_files DEPENDS ${SAFTLIB_GENERATED_FILES})
# Create a custom target to ensure generation of all service files
add_custom_target(service_files DEPENDS ${SERVICE_GENERATED_SOURCES})
# Create a custom target to ensure generation of all proxy files
add_custom_target(proxy_files DEPENDS ${PROXY_GENERATED_SOURCES})

generate_saftbus_files( "${GENERATOR_SOURCES}" )

# MESSAGE (STATUS "SAFTLIB_GENERATOR_DIR: ${SAFTLIB_GENERATOR_DIR}")
# MESSAGE (STATUS "SERVICE_GENERATED_SOURCES: ${SERVICE_GENERATED_SOURCES} \n")
# MESSAGE (STATUS "PROXY_GENERATED_SOURCES: ${PROXY_GENERATED_SOURCES} \n")
# MESSAGE (STATUS "SAFTLIB_GENERATED_FILES: ${SAFTLIB_GENERATED_FILES} \n")

# ============================================================================
# saft services
# ============================================================================

add_library(saft-service SHARED)

target_sources( saft-service

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Time.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/eb-source.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/eb-forward.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/build.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MasterFunctionGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorImpl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorFirmware.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/BurstGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/build.cpp
        ${SERVICE_GENERATED_SOURCES}

    # needs to be checked individually - should not be all public
    PUBLIC
    FILE_SET saft_service_public_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ats_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/bg_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/BuildIdRom.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/BurstGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/burstgen_shared_mmap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/CommonFunctions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/CommonFunctions.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Condition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eb-forward.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eb-source.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_ac_wbm_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_Event.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_flags.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_queue_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_TLU.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_tlu_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Error.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EventSource.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fg_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGeneratorFirmware.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGeneratorImpl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Input.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/IoControl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/io_control_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Io.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/LM32Cluster.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Mailbox.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/MasterFunctionGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/MsiDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/OpenDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/OutputCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Output.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Owned.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Reset.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SAFTd.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/saft-tools-define.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SdbDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SerdesClockGen.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SignalGroup.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TempSensor.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Time.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Time.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TimingReceiverAddon.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TimingReceiver.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Watchdog.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmActionCmd.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WhiteRabbit.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wr_mil_gw_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/build.hpp

    # needs to be checked individually - should not be all public
    PUBLIC
    FILE_SET saft_service_generated_headers
    TYPE    HEADERS
    BASE_DIRS ${SAFTLIB_GENERATOR_DIR}
    FILES
        ${SERVICE_GENERATED_HEADERS}

    PUBLIC
    FILE_SET saft_service_interface_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/BurstGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/CommonFunctions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/EmbeddedCPUActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/EmbeddedCPUCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/FunctionGeneratorFirmware.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/FunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iBurstGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iDevice.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEmbeddedCPUActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEmbeddedCPUCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEventSource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iFunctionGeneratorFirmware.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iInputEventSource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMasterFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMILbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMILbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/Input.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOutputActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOutputCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOwned.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSAFTd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSCUbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSCUbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSoftwareActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSoftwareCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iTimingReceiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWbmActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWbmCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWrMilGateway.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MasterFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MILbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MILbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/OutputCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/Output.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/saftbus.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SAFTd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/saftlib.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SCUbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SCUbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SoftwareActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SoftwareCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/TimingReceiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WbmActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WbmCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WrMilGateway.h
)

add_dependencies(saft-service all_generated_files)

set_target_properties(saft-service PROPERTIES
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    SOVERSION ${SONAME}
)

# Should be put into target_sources above
target_include_directories(saft-service
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saft-service
    PUBLIC
        saftbus
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        dl
)


# ============================================================================
# saft proxies
# ============================================================================

add_library(saft-proxy SHARED)

target_sources( saft-proxy

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Time.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CommonFunctions.cpp
        ${PROXY_GENERATED_SOURCES}

    # needs to be checked individually - should not be all public
    PUBLIC
    FILE_SET saft_proxy_public_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ats_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/bg_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/BuildIdRom.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/BurstGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/burstgen_shared_mmap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/CommonFunctions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/CommonFunctions.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Condition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eb-forward.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eb-source.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_ac_wbm_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_Event.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_flags.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_queue_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ECA_TLU.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/eca_tlu_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedCPUCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Error.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EventSource.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fg_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGeneratorFirmware.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/FunctionGeneratorImpl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Input.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/IoControl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/io_control_regs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Io.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/LM32Cluster.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Mailbox.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/MasterFunctionGenerator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/MsiDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/OpenDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/OutputCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Output.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Owned.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Reset.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SAFTd.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/saft-tools-define.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SCUbusCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SdbDevice.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SerdesClockGen.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SignalGroup.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SoftwareCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TempSensor.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Time.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Time.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TimingReceiverAddon.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/TimingReceiver.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Watchdog.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmActionCmd.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmActionSink.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WbmCondition.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/WhiteRabbit.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/wr_mil_gw_regs.h

    # needs to be checked individually - should not be all public
    PUBLIC
    FILE_SET saft_proxy_generated_headers
    TYPE    HEADERS
    BASE_DIRS ${SAFTLIB_GENERATOR_DIR}
    FILES
        ${PROXY_GENERATED_HEADERS}

    INTERFACE
    FILE_SET saft_proxy_interface_headers
    TYPE    HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/BurstGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/CommonFunctions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/EmbeddedCPUActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/EmbeddedCPUCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/FunctionGeneratorFirmware.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/FunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iBurstGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iDevice.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEmbeddedCPUActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEmbeddedCPUCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iEventSource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iFunctionGeneratorFirmware.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iInputEventSource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMasterFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMILbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iMILbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/Input.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOutputActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOutputCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iOwned.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSAFTd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSCUbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSCUbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSoftwareActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iSoftwareCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iTimingReceiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWbmActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWbmCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/iWrMilGateway.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MasterFunctionGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MILbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/MILbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/OutputCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/Output.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/saftbus.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SAFTd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/saftlib.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SCUbusActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SCUbusCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SoftwareActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/SoftwareCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/TimingReceiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WbmActionSink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WbmCondition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces/WrMilGateway.h
)

add_dependencies(saft-proxy all_generated_files)

set_target_properties(saft-proxy  PROPERTIES
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    SOVERSION ${SONAME}
)

# Should be put into target_sources above
target_include_directories(saft-proxy
    PRIVATE
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

target_link_libraries(saft-proxy
    PUBLIC
        saftbus
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        dl
)


# ============================================================================
# Function Generator
# ============================================================================

# Service

add_library( fg-firmware-service SHARED )

target_sources( fg-firmware-service

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGenerator.cpp
        ${SAFTLIB_GENERATOR_DIR}/FunctionGenerator_Service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MasterFunctionGenerator.cpp
        ${SAFTLIB_GENERATOR_DIR}/MasterFunctionGenerator_Service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorImpl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorFirmware.cpp
        ${SAFTLIB_GENERATOR_DIR}/FunctionGeneratorFirmware_Service.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fg_firmware_create_service.cpp
)

add_dependencies(fg-firmware-service saft-service)

target_link_libraries(fg-firmware-service
    PUBLIC
        ${SIGCPP_LIBS}
        saftbus
        saft-service
        dl
)

target_compile_options(fg-firmware-service
    PRIVATE
        ${SIGCPP_CFLAGS}
)

target_include_directories(fg-firmware-service
    PRIVATE
        ${SAFTLIB_GENERATOR_DIR}
        ${SIGCPP_INCLUDE_DIRS}
)

# Proxy

add_library(fg-firmware-proxy SHARED)

target_sources(fg-firmware-proxy

    PRIVATE
    	${SAFTLIB_GENERATOR_DIR}/FunctionGenerator_Proxy.cpp
    	${SAFTLIB_GENERATOR_DIR}/MasterFunctionGenerator_Proxy.cpp
    	${SAFTLIB_GENERATOR_DIR}/FunctionGeneratorFirmware_Proxy.cpp
)

add_dependencies(fg-firmware-proxy saft-proxy)

target_link_libraries(fg-firmware-proxy
    PUBLIC
        ${SIGCPP_LIBS}
        saftbus
        saft-service
        dl
)

target_compile_options(fg-firmware-proxy
    PRIVATE
        ${SIGCPP_CFLAGS}
)

target_include_directories(fg-firmware-proxy
    PRIVATE
        ${SAFTLIB_GENERATOR_DIR}
        ${SIGCPP_INCLUDE_DIRS}
)


# ============================================================================
# Burst Generator
# ============================================================================

# Service

add_library(bg-firmware-service SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BurstGenerator.cpp
	${SAFTLIB_GENERATOR_DIR}/BurstGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/bg_firmware_create_service.cpp
)

add_dependencies(bg-firmware-service saft-service)

target_link_libraries(bg-firmware-service
    PUBLIC
        ${SIGCPP_LIBS}
        saftbus
        saft-service
        dl
)

target_compile_options(bg-firmware-service
    PRIVATE
        ${SIGCPP_CFLAGS}
)

target_include_directories(bg-firmware-service
    PRIVATE
        ${SAFTLIB_GENERATOR_DIR}
        ${SIGCPP_INCLUDE_DIRS}
)

target_compile_definitions(bg-firmware-service PRIVATE
    DATADIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Proxy

add_library(bg-firmware-proxy SHARED
	${SAFTLIB_GENERATOR_DIR}/BurstGenerator_Proxy.cpp
)

add_dependencies(bg-firmware-proxy saft-proxy)

target_link_libraries(bg-firmware-proxy
    PUBLIC
        ${SIGCPP_LIBS}
        saftbus
        saft-service
        dl
)

target_compile_options(bg-firmware-proxy
    PRIVATE
        ${SIGCPP_CFLAGS}
)

target_include_directories(bg-firmware-proxy
    PRIVATE
        ${SAFTLIB_GENERATOR_DIR}
        ${SIGCPP_INCLUDE_DIRS}
)


# ============================================================================
# Package Configuration
# ============================================================================

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    saftlib.cmake
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion
)

# ============================================================================
# Installation
# ============================================================================

install(
    TARGETS saft-service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_service_public_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_service_generated_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_service_interface_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
)

install(
    TARGETS saft-proxy
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_proxy_public_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_proxy_generated_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
    FILE_SET saft_proxy_interface_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/saftlib/
)

install(
    TARGETS
        fg-firmware-service
        fg-firmware-proxy
        bg-firmware-service
        bg-firmware-proxy
)

