# ============================================================================
# Code Generation Function
# ============================================================================

function(get_env
)

# Get username, hostname, and operating system
if(UNIX)
    # Use environment variables or system commands to get these
    execute_process(
        COMMAND whoami
        OUTPUT_VARIABLE USERNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND uname -s
        OUTPUT_VARIABLE OPERATING_SYSTEM
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Fallback for non-Unix systems
    set(USERNAME "$ENV{USERNAME}")
    set(HOSTNAME "$ENV{COMPUTERNAME}")
    set(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")
endif()

set(USERNAME            "${USERNAME}" PARENT_SCOPE)
set(HOSTNAME            "${HOSTNAME}" PARENT_SCOPE)
set(OPERATING_SYSTEM    "${OPERATING_SYSTEM}" PARENT_SCOPE)

endfunction()


# TODO: remove build.*pp & version.h
# echo '#define GIT_ID "'$(git describe --dirty --always --tags)'"' > version.h.tmp
# echo '#define SOURCE_DATE "'$(date +'%b %d %Y %H:%M:%S' --date=@$(git log -n1 --pretty='format:%ct'))'"' >> version.h.tmp
