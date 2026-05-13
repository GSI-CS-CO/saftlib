cmake_minimum_required(VERSION 3.21...4.0)

set(MAJOR "3")      # Increment if removed/changed properties/signals/methods since previous release
set(MINOR "2")      # Increment if added properties/signals/methods; reset to 0 if MAJOR changed
set(REVISION "3")   # Increment on each release; reset to 0 if MAJOR/MINOR changed
set(SONAME "10")    # Whenever MAJOR is incremented, add MINOR+1 to this variable

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

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(EB REQUIRED etherbone>=2.1.0)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(LIBRTPI REQUIRED librtpi>=1.0.1)


# ============================================================================
# Software Timing Receiver
# ===========================================================================

# add_executable( saft-software-tr
#   saft-software-tr.cpp
# )

# target_link_libraries( saft-software-tr
#     PRIVATE
#         ${EB_LIBS}
#         pthread
#         dl
# )

# target_compile_options( saft-software-tr
#     PRIVATE
#         sdb_DATA=software-tr.sdb
# )

# # software timing receiver program
# # saft_software_tr_sdb_DATA = src/software-tr.sdb
# # saft_software_tr_sdbdir   = $(datadir)/saftlib
# # saft_software_tr_LDADD    = $(EB_LIBS) -lpthread  -ldl #-lltdl
# # saft_software_tr_SOURCES  = src/saft-software-tr.cpp


# ============================================================================
# Another Software Timing Receiver
# ===========================================================================

# add_executable( soft-tr
#   soft-tr.cpp
# )

# target_link_libraries( soft-tr
#     PRIVATE
#         ${EB_LIBS}
#         ${SIGCPP_LIBS}
#         saftbus
#         saft_service
#         dl
# )


# ============================================================================
# simple program to find out how to receive MSIs from soft-tr
# ===========================================================================

add_executable( wait-msi
  wait-msi.cpp
)

target_link_libraries( wait-msi
    PRIVATE
        saftbus
        saft-service
        dl
)

# ============================================================================
# Saft Testbench
# ===========================================================================

add_executable( saft-testbench
  saft-testbench.cpp
)

target_link_libraries( saft-testbench
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft CTL
# ===========================================================================

add_executable( saft-ctl
  saft-ctl.cpp
)

target_link_libraries( saft-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft IO-CTL
# ===========================================================================

add_executable( saft-io-ctl
  saft-io-ctl.cpp
)

target_link_libraries( saft-io-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft PPs Generator
# ===========================================================================

add_executable( saft-pps-gen
  saft-pps-gen.cpp
)

target_link_libraries( saft-pps-gen
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft SCU CTL
# ===========================================================================

add_executable( saft-scu-ctl
  saft-scu-ctl.cpp
)

target_link_libraries( saft-scu-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft E-CPU CTL
# ===========================================================================

add_executable( saft-epcu-ctl
  saft-ecpu-ctl.cpp
)

target_link_libraries( saft-epcu-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft WBM CTL
# ===========================================================================

add_executable( saft-wbm-ctl
  saft-wbm-ctl.cpp
)

target_link_libraries( saft-wbm-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft Clock Generator
# ===========================================================================

add_executable( saft-clk-gen
  saft-clk-gen.cpp
)

target_link_libraries( saft-clk-gen
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft DM
# ===========================================================================

add_executable( saft-dm
  saft-dm.cpp
)

target_link_libraries( saft-dm
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft EB Forward
# ===========================================================================

add_executable( saft-eb-fwd
  saft-eb-fwd.cpp
)

target_link_libraries( saft-eb-fwd
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft GMT Check
# ===========================================================================

add_executable( saft-gmt-check
  saft-gmt-check.cpp
)

target_link_libraries( saft-gmt-check
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft Uni
# ===========================================================================

add_executable( saft-uni
  saft-uni.cpp
)

target_link_libraries( saft-uni
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# ============================================================================
# Saft LCD
# ===========================================================================

add_executable( saft-lcd
  saft-lcd.cpp
)

target_link_libraries( saft-lcd
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# # ============================================================================
# # Saft Standlone Mailbox
# # ===========================================================================

# add_executable( saft-standalone-mbox
#   saft-standalone-mbox.cpp
# )

# target_link_libraries( saft-standalone-mbox
#     PRIVATE
#         ${EB_LIBS}
#         ${SIGCPP_LIBS}
#         saftbus
#         saft-service
#         dl
# )


# ============================================================================
# Saft Roundtrip Latency Measurement
# ===========================================================================

add_executable( saft-roundtrip-latency
  saft-roundtrip-latency.cpp
)

target_link_libraries( saft-roundtrip-latency
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        dl
)


# # ============================================================================
# # Saft Standalone Roundtrip Latency Measurement
# # ===========================================================================

# add_executable( saft-standalone-roundtrip-latency
#   saft-standalone-roundtrip-latency.cpp
# )

# target_link_libraries( saft-standalone-roundtrip-latency
#     PRIVATE
#         ${EB_LIBS}
#         ${SIGCPP_LIBS}
#         saftbus
#         saft-service
#         dl
# )


# ============================================================================
# Saft Function Generator CTL
# ===========================================================================

add_executable( saft-fg-ctl
  saft-fg-ctl.cpp
)

target_link_libraries( saft-fg-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        saft-fg-proxy
        dl
)


# ============================================================================
# Saft Multi Function Generator CTL
# ===========================================================================

add_executable( saft-mfg-ctl
  saft-mfg-ctl.cpp
)

target_link_libraries( saft-mfg-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        saft-fg-proxy
        pthread
        dl
)


# ============================================================================
# Saft Burst CTL
# ===========================================================================

add_executable( saft-burst-ctl
  saft-burst-ctl.cpp
)

target_link_libraries( saft-burst-ctl
    PRIVATE
        ${EB_LIBS}
        ${SIGCPP_LIBS}
        saftbus
        saft-proxy
        saft-bg-proxy
        dl
)
