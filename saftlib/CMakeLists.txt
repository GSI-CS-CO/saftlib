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

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(EB REQUIRED etherbone>=2.1.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(LIBRTPI REQUIRED librtpi>=1.0.1)


# ============================================================================
# Env Foo
# ===========================================================================
include(${CMAKE_SOURCE_DIR}/cmake/environment.cmake)
# Get username, hostname, and operating system
get_env()
# Add preprocessor definitions
add_compile_definitions(
    PACKAGE_STRING="${PROJECT_NAME} ${PROJECT_VERSION}"
    USERNAME="${USERNAME}"
    HOSTNAME="${HOSTNAME}"
    OPERATING_SYSTEM="${OPERATING_SYSTEM}"
    GIT_ID="${GIT_ID}"
    SOURCE_DATE="${SOURCE_DATE}"
)


# ============================================================================
# Get generator function
# ============================================================================
include(${CMAKE_SOURCE_DIR}/cmake/generator.cmake)

# ============================================================================
# Saftbus generator mechanics
# ============================================================================

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


# Set vars now to track all generated files
set(SERVICE_GENERATED_SOURCES)
set(PROXY_GENERATED_SOURCES)
set(SAFTLIB_GENERATED_FILES)


# Create a custom target to ensure generation of all service files
add_custom_target(all_generated_files DEPENDS ${SAFTLIB_GENERATED_FILES})
# Create a custom target to ensure generation of all service files
#add_custom_target(service_files DEPENDS ${SERVICE_GENERATED_SOURCES})
# Create a custom target to ensure generation of all proxy files
#add_custom_target(proxy_files DEPENDS ${PROXY_GENERATED_SOURCES})

add_custom_target( generate_sources DEPENDS all_generated_files)
generate_saftbus_files( "${GENERATOR_SOURCES}" generate_sources )

#MESSAGE (STATUS "SERVICE_GENERATED_SOURCES: ${SERVICE_GENERATED_SOURCES} \n")
#MESSAGE (STATUS "PROXY_GENERATED_SOURCES: ${PROXY_GENERATED_SOURCES} \n")
#MESSAGE (STATUS "SAFTLIB_GENERATED_FILES: ${SAFTLIB_GENERATED_FILES} \n")

# ============================================================================
# saft services
# ============================================================================

# Source files
set(LIBSAFT_SERVICE_GEN_SOURCES
    ${CMAKE_BINARY_DIR}/src/generated/ActionSink_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/BuildIdRom_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Condition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_Event_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_TLU_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EmbeddedCPUCondition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EmbeddedCPUActionSink_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EventSource_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Input_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/LM32Cluster_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/OpenDevice_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Output_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/OutputCondition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Owned_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Reset_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SAFTd_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SCUbusActionSink_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SCUbusCondition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SoftwareActionSink_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SoftwareCondition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/TempSensor_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/TimingReceiver_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WbmActionSink_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WbmCondition_Service.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WhiteRabbit_Service.cpp
)

set(LIBSAFT_SERVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/build.cpp
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
    ${LIBSAFT_SERVICE_GEN_SOURCES}
)

add_library(saft-service SHARED ${LIBSAFT_SERVICE_SOURCES})

add_dependencies(saft-service generate_sources)

set_target_properties(saft-service PROPERTIES
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    SOVERSION ${SONAME}
)

target_include_directories(saft-service
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/src/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
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

set(LIBSAFT_PROXY_GEN_SOURCES
    ${CMAKE_BINARY_DIR}/src/generated/ActionSink_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/BuildIdRom_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Condition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_Event_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/ECA_TLU_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EmbeddedCPUCondition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EmbeddedCPUActionSink_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/EventSource_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Input_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/LM32Cluster_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/OpenDevice_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Output_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/OutputCondition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Owned_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/Reset_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SAFTd_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SCUbusActionSink_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SCUbusCondition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SoftwareActionSink_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/SoftwareCondition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/TempSensor_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/TimingReceiver_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WbmActionSink_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WbmCondition_Proxy.cpp
    ${CMAKE_BINARY_DIR}/src/generated/WhiteRabbit_Proxy.cpp
)

set(LIBSAFT_PROXY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Time.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/CommonFunctions.cpp
    ${LIBSAFT_PROXY_GEN_SOURCES}
)

add_library(saft-proxy SHARED ${LIBSAFT_PROXY_SOURCES})

add_dependencies(saft-proxy generate_sources)

set_target_properties(saft-proxy  PROPERTIES
    VERSION ${MAJOR}.${MINOR}.${REVISION}
    SOVERSION ${SONAME}
)

target_include_directories(saft-proxy
    PUBLIC
        ${CMAKE_BINARY_DIR}/src/generated/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
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

add_library(fg-firmware-service SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGenerator.cpp
	${CMAKE_BINARY_DIR}/src/generated/FunctionGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/MasterFunctionGenerator.cpp
	${CMAKE_BINARY_DIR}/src/generated/MasterFunctionGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorImpl.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorFirmware.cpp
	${CMAKE_BINARY_DIR}/src/generated/FunctionGeneratorFirmware_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/fg_firmware_create_service.cpp
)

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
        ${CMAKE_BINARY_DIR}/src/generated/
        ${SIGCPP_INCLUDE_DIRS}
)

# Proxy

add_library(fg-firmware-proxy SHARED
	${CMAKE_BINARY_DIR}/src/generated/FunctionGenerator_Proxy.cpp
	${CMAKE_BINARY_DIR}/src/generated/MasterFunctionGenerator_Proxy.cpp
	${CMAKE_BINARY_DIR}/src/generated/FunctionGeneratorFirmware_Proxy.cpp
)

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
        ${CMAKE_BINARY_DIR}/src/generated/
        ${SIGCPP_INCLUDE_DIRS}
)


# ============================================================================
# Burst Generator
# ============================================================================

# Service

add_library(bg-firmware-service SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BurstGenerator.cpp
	${CMAKE_BINARY_DIR}/src/generated/BurstGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/bg_firmware_create_service.cpp
)

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
        ${CMAKE_BINARY_DIR}/src/generated/
        ${SIGCPP_INCLUDE_DIRS}
)

target_compile_definitions(bg-firmware-service PRIVATE
    DATADIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Proxy

add_library(bg-firmware-proxy SHARED
	${CMAKE_BINARY_DIR}/src/generated/BurstGenerator_Proxy.cpp
)

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
        ${CMAKE_BINARY_DIR}/src/generated/
        ${SIGCPP_INCLUDE_DIRS}
)
