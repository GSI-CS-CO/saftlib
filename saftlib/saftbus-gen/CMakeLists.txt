cmake_minimum_required(VERSION 3.21...4.0)

# ============================================================================
# saftbus-gen Code Generator
# ============================================================================

project(saftbus-gen LANGUAGES CXX)

# Option to use local saftbus-gen if system version not found
option(NO_SAFTBUSGEN "Build local saftbus-gen if system version not found" ON)

# Try to find system saftbus-gen first
find_program(SAFTBUSGEN_EXECUTABLE saftbus-gen)

if(NOT SAFTBUSGEN_EXECUTABLE)
    add_executable(saftbus-gen saftbus-gen.cpp)

    set_target_properties(saftbus-gen PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
    )

    target_compile_definitions(saftbus-gen PRIVATE VERSION="local")

    target_compile_options(saftbus-gen PRIVATE -O2 -g)

    set(SAFTBUSGEN_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/saftbus-gen-local)

else()
    # Use system saftbus-gen
    find_program(SAFTBUSGEN_EXECUTABLE saftbus-gen REQUIRED)
endif()

# ============================================================================
# Code Generation Function
# ============================================================================

function(generate_saftbus_files INPUT_FILES)

foreach(SRC_FILE ${INPUT_FILES})

    get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

    set(OUTPUT_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/generated/${BASENAME}_Service.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/generated/${BASENAME}_Proxy.hpp
    )

    add_custom_command(
        OUTPUT ${OUTPUT_FILES}
        COMMAND ${SAFTBUSGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE} -o ${CMAKE_CURRENT_BINARY_DIR}/src
        DEPENDS ${INPUT_FILE} ${SAFTBUSGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating saftbus code from ${INPUT_FILE}"
    )
endforeach()

    # Return the output files to caller
    set(GENERATED_FILES ${OUTPUT_FILES} PARENT_SCOPE)
endfunction()

