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
)

# ============================================================================
# Sources for generator
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

MESSAGE (STATUS "SERVICE_GENERATED_SOURCES: ${SERVICE_GENERATED_SOURCES} \n")
MESSAGE (STATUS "PROXY_GENERATED_SOURCES: ${PROXY_GENERATED_SOURCES} \n")
MESSAGE (STATUS "SAFTLIB_GENERATED_FILES: ${SAFTLIB_GENERATED_FILES} \n")

# ============================================================================
# saft services
# ============================================================================

# Source files
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
    ${SERVICE_GENERATED_SOURCES}
)

# Create the shared library
add_library(saft_service SHARED ${LIBSAFT_SERVICE_SOURCES})

add_dependencies(saft_service generate_sources)

# Set library properties
set_target_properties(saft_service PROPERTIES
    VERSION @SAFTD_API@.@SAFTD_REVISION@.@SAFTD_MINOR@
    SOVERSION @SAFTD_API@
    LINK_FLAGS "-Wl,--export-dynamic"
)

target_include_directories(saft_service
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/src/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
    PRIVATE
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


# ============================================================================
# saft proxies
# ============================================================================

# Source files
set(LIBSAFT_PROXY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Time.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/CommonFunctions.cpp
    ${PROXY_GENERATED_SOURCES}
)

# Create the shared library
add_library(saft_proxy SHARED ${LIBSAFT_PROXY_SOURCES})

add_dependencies(saft_proxy generate_sources)

# Set library properties
set_target_properties(saft_proxy PROPERTIES
    VERSION @SAFTD_API@.@SAFTD_REVISION@.@SAFTD_MINOR@
    SOVERSION @SAFTD_API@
    LINK_FLAGS "-Wl,--export-dynamic"
)

target_include_directories(saft_proxy
    PUBLIC
        ${CMAKE_BINARY_DIR}/src/generated/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
    PRIVATE
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(saft_proxy
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        dl
)


# ============================================================================
# libfg service
# ============================================================================

# Source files
set(LIBSAFT_FG_SERVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGenerator.cpp
	${CMAKE_BINARY_DIR}/src/generated/FunctionGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/MasterFunctionGenerator.cpp
	${CMAKE_BINARY_DIR}/src/generated/MasterFunctionGenerator_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorImpl.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionGeneratorFirmware.cpp
	${CMAKE_BINARY_DIR}/src/generated/FunctionGeneratorFirmware_Service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/fg_firmware_create_service.cpp
)

# Function Generator Firmware Service Library
add_library(saft_fg_service SHARED
    ${LIBSAFT_FG_SERVICE_SOURCES}
)

target_link_libraries(saft_fg_service
    PUBLIC
        ${SIGCPP_LIBS}
        saftbus
        saft_service
        dl
)

target_compile_options(saft_fg_service
    PRIVATE
        ${SIGCPP_CFLAGS}
)

target_link_options(saft_fg_service
    PRIVATE
        "LINKER:--export-dynamic"
)

# Set library properties
set_target_properties(saft_fg_service PROPERTIES
    VERSION @SAFTD_API@.@SAFTD_REVISION@.@SAFTD_MINOR@
    SOVERSION @SAFTD_API@
    LINK_FLAGS "-Wl,--export-dynamic"
)

target_include_directories(saft_fg_service
    PUBLIC
        ${CMAKE_BINARY_DIR}/src/generated/
    PRIVATE
        ${SIGCPP_INCLUDE_DIRS}
        ${LIBRTPI_INCLUDE_DIRS}
)


