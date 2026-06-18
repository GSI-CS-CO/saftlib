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

    execute_process(
        COMMAND git describe --dirty --always --tags
        OUTPUT_VARIABLE GIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND git log -n1 --pretty=format:%ct
        OUTPUT_VARIABLE COMMIT_TIMESTAMP
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND date +"%-b %-d %Y %H:%M:%S" --date=@${COMMIT_TIMESTAMP}
        OUTPUT_VARIABLE SOURCE_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    # Fallback for non-Unix systems
    set(USERNAME "$ENV{USERNAME}")
    set(HOSTNAME "$ENV{COMPUTERNAME}")
    set(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")
endif()

set(USERNAME            ${USERNAME} PARENT_SCOPE)
set(HOSTNAME            ${HOSTNAME} PARENT_SCOPE)
set(OPERATING_SYSTEM    ${OPERATING_SYSTEM} PARENT_SCOPE)
set(GIT_ID              ${GIT_ID} PARENT_SCOPE)
set(SOURCE_DATE         ${SOURCE_DATE} PARENT_SCOPE)

endfunction()
