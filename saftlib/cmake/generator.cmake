# ============================================================================
# Code Generation Function
# ============================================================================

function(generate_saftbus_files
    INPUT_FILES
)

make_directory(${SAFTLIB_GENERATOR_DIR})

foreach(SRC_FILE ${INPUT_FILES})

    get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

    set(OUTPUT_FILES
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.cpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.hpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.cpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.hpp
    )

    list(APPEND SAFTLIB_GENERATED_FILES
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.cpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.hpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.cpp
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.hpp
    )

    # Add generated files to lists
    list(APPEND SERVICE_GENERATED_SOURCES
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.cpp
    )
    list(APPEND SERVICE_GENERATED_HEADERS
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Service.hpp
    )
    list(APPEND PROXY_GENERATED_SOURCES
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.cpp
    )
    list(APPEND PROXY_GENERATED_HEADERS
        ${SAFTLIB_GENERATOR_DIR}/${BASENAME}_Proxy.hpp
    )

    add_custom_command(
        OUTPUT ${OUTPUT_FILES}
        COMMAND ${SAFTBUSGEN_EXECUTABLE} "${SRC_FILE}" -o "${SAFTLIB_GENERATOR_DIR}" -I "${CMAKE_CURRENT_SOURCE_DIR}/include" -I "${SAFTLIB_GENERATOR_DIR}"
        COMMENT "Generating saftbus code from ${SRC_FILE}"
    )

endforeach()

# Return the output files to caller
set(SAFTLIB_GENERATED_FILES ${SAFTLIB_GENERATED_FILES} PARENT_SCOPE)
set(SERVICE_GENERATED_SOURCES  ${SERVICE_GENERATED_SOURCES} PARENT_SCOPE)
set(SERVICE_GENERATED_HEADERS  ${SERVICE_GENERATED_HEADERS} PARENT_SCOPE)
set(PROXY_GENERATED_SOURCES ${PROXY_GENERATED_SOURCES} PARENT_SCOPE)
set(PROXY_GENERATED_HEADERS ${PROXY_GENERATED_HEADERS} PARENT_SCOPE)

endfunction()
