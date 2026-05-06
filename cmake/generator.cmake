# ============================================================================
# Code Generation Function
# ============================================================================

function(generate_saftbus_files
    INPUT_FILES
    GIVEN_TARGET
)

foreach(SRC_FILE ${INPUT_FILES})

    get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

    set(OUTPUT_FILES
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
        ${CMAKE_BINARY_DIR}/include/generated/${BASENAME}_Service.hpp
        ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
        ${CMAKE_BINARY_DIR}/include/generated/${BASENAME}_Proxy.hpp
    )
    make_directory("${CMAKE_BINARY_DIR}/src/generated/")

    #MESSAGE (STATUS "${OUTPUT_FILES}")
    #MESSAGE (STATUS "${SAFTBUSGEN_EXECUTABLE}")
    add_custom_command(
        DEPENDS ${SRC_FILE}
        OUTPUT ${OUTPUT_FILES}
        COMMAND ${SAFTBUSGEN_EXECUTABLE} "${SRC_FILE}" -o "${CMAKE_BINARY_DIR}/src/generated/" -I ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DEPENDS ${INPUT_FILE} ${SAFTBUSGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating saftbus code from ${INPUT_FILE}"
    )
    #MESSAGE (STATUS "${SAFTBUSGEN_EXECUTABLE} ${SRC_FILE} -o ${CMAKE_BINARY_DIR}/src/generated/")
endforeach()

# Return the output files to caller
set(SAFTLIB_GENERATED_FILES ${OUTPUT_FILES} PARENT_SCOPE)
endfunction()
