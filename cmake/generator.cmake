# ============================================================================
# generate lists of generated files
# ============================================================================
# function(what_files_to_generate
#     INPUT_FILES
# )

# foreach(SRC_FILE ${INPUT_FILES})

#     MESSAGE (STATUS "${SRC_FILE}")

#     get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

#     # Add generated files to lists
#     list(APPEND SERVICE_GENERATED_SOURCES
#         ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
#         ${CMAKE_BINARY_DIR}/include/generated/${BASENAME}_Service.hpp
#     )
#     list(APPEND PROXY_GENERATED_SOURCES
#         ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
#         ${CMAKE_BINARY_DIR}/include/generated/${BASENAME}_Proxy.hpp
#     )

# endforeach()

# endfunction()

# # Generate Service and Proxy files
# foreach(HEADER ${SAFTBUS_GENERATE_HEADERS})
#     get_filename_component(BASE_NAME ${HEADER} NAME_WE)

#     # Service files generation
#     add_custom_command(
#         OUTPUT
#             ${CMAKE_CURRENT_SOURCE_DIR}/src/${BASE_NAME}_Service.cpp
#             ${CMAKE_CURRENT_SOURCE_DIR}/src/${BASE_NAME}_Service.hpp
#         COMMAND
#             ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lock_dir
#         COMMAND
#             flock ${CMAKE_CURRENT_BINARY_DIR}/lock_dir ${SAFTBUS_GEN} ${HEADER} -o src
#         COMMAND
#             ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/lock_dir
#         DEPENDS
#             ${HEADER}
#             ${SAFTBUS_GEN}
#     )

#     # Proxy files generation
#     add_custom_command(
# 		OUTPUT
#             ${CMAKE_CURRENT_SOURCE_DIR}/src/${BASE_NAME}_Proxy.cpp
#             ${CMAKE_CURRENT_SOURCE_DIR}/src/${BASE_NAME}_Proxy.hpp
#         COMMAND
#             ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lock_dir
#         COMMAND
#             flock ${CMAKE_CURRENT_BINARY_DIR}/lock_dir ${SAFTBUS_GEN} ${HEADER} -o src
#         COMMAND
#             ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/lock_dir
#         DEPENDS
#             ${HEADER}
#             ${SAFTBUS_GEN}
#     )

#     # Add generated files to lists
#     list(APPEND SERVICE_GENERATED_SOURCES
#         src/${BASE_NAME}_Service.cpp
#         src/${BASE_NAME}_Service.hpp
#     )
#     list(APPEND PROXY_GENERATED_SOURCES
#         src/${BASE_NAME}_Proxy.cpp
#         src/${BASE_NAME}_Proxy.hpp
#     )

# ============================================================================
# generate list source to be generated
# ============================================================================



# ============================================================================
# Code Generation Function
# ============================================================================

function(generate_saftbus_files
    INPUT_FILES
    GIVEN_TARGET
)

make_directory("${CMAKE_BINARY_DIR}/src/generated/")

foreach(SRC_FILE ${INPUT_FILES})

    MESSAGE (STATUS "${SRC_FILE}")

    get_filename_component(BASENAME ${SRC_FILE} NAME_WE)

    # set(OUTPUT_FILES
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    # )

    # list(APPEND SAFTLIB_GENERATED_FILES
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    # )

    # # Add generated files to lists
    # list(APPEND SERVICE_GENERATED_SOURCES
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Service.hpp
    # )
    # list(APPEND PROXY_GENERATED_SOURCES
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.cpp
    #     ${CMAKE_BINARY_DIR}/src/generated/${BASENAME}_Proxy.hpp
    # )

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

    #MESSAGE (STATUS "${OUTPUT_FILES}")
    #MESSAGE (STATUS "${SAFTBUSGEN_EXECUTABLE}")
    add_custom_command(
        DEPENDS ${SRC_FILE}
        OUTPUT ${OUTPUT_FILES}
        #COMMAND ${SAFTBUSGEN_EXECUTABLE} "${SRC_FILE}" -o "${CMAKE_BINARY_DIR}/src/generated" -I ${CMAKE_CURRENT_SOURCE_DIR}/include -I ${CMAKE_BINARY_DIR}/src/generated
        COMMAND ${SAFTBUSGEN_EXECUTABLE} "${SRC_FILE}" -o src -I ${CMAKE_CURRENT_SOURCE_DIR}/include
        DEPENDS ${INPUT_FILES} ${SAFTBUSGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating saftbus code from ${SRC_FILE}"
    )
    #MESSAGE (STATUS "${SAFTBUSGEN_EXECUTABLE} ${SRC_FILE} -o ${CMAKE_BINARY_DIR}/src/generated/ -I ${CMAKE_CURRENT_SOURCE_DIR}/include/")
endforeach()

# Return the output files to caller
set(SAFTLIB_GENERATED_FILES ${SAFTLIB_GENERATED_FILES} PARENT_SCOPE)
set(SERVICE_GENERATED_SOURCES  ${SERVICE_GENERATED_SOURCES} PARENT_SCOPE)
set(PROXY_GENERATED_SOURCES ${PROXY_GENERATED_SOURCES} PARENT_SCOPE)
MESSAGE (STATUS "${OUTPUT_FILES}")

endfunction()
