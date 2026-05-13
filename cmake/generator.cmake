# ============================================================================
# Code Generation Function
# ============================================================================

function(generate_saftbus_files
    INPUT_FILES
)

make_directory("${CMAKE_BINARY_DIR}/src/generated/")

foreach(SRC_FILE ${INPUT_FILES})

    get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

    set(OUTPUT_FILES
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    )

    list(APPEND SAFTLIB_GENERATED_FILES
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    )

    # Add generated files to lists
    list(APPEND SERVICE_GENERATED_SOURCES
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
    )
    list(APPEND PROXY_GENERATED_SOURCES
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    )

    make_directory("${CMAKE_BINARY_DIR}/src/generated/")

    add_custom_command(
        OUTPUT ${OUTPUT_FILES}
        COMMAND ${SAFTBUSGEN_EXECUTABLE} "${SRC_FILE}" -o "${CMAKE_BINARY_DIR}/src/generated" -I "${CMAKE_CURRENT_SOURCE_DIR}/include" -I "${CMAKE_BINARY_DIR}/src/generated"
        COMMENT "Generating saftbus code from ${SRC_FILE}"
    )

endforeach()

# Return the output files to caller
set(SAFTLIB_GENERATED_FILES ${SAFTLIB_GENERATED_FILES} PARENT_SCOPE)
set(SERVICE_GENERATED_SOURCES  ${SERVICE_GENERATED_SOURCES} PARENT_SCOPE)
set(PROXY_GENERATED_SOURCES ${PROXY_GENERATED_SOURCES} PARENT_SCOPE)

endfunction()
