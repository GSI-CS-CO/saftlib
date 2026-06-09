set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_minimum_required(VERSION 3.23...4.0)

# ============================================================================
# saftbus-gen Code Generator
# ============================================================================

project(saftbus-gen LANGUAGES CXX)

add_executable(saftbus-gen saftbus-gen.cpp)

set_target_properties(saftbus-gen PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_compile_definitions(saftbus-gen PRIVATE VERSION="local")
target_compile_options(saftbus-gen PRIVATE -O2 -g)
set(SAFTBUSGEN_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/saftbus-gen PARENT_SCOPE)





