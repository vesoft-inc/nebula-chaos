# Copyright (c) 2020 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#

find_program(ECHO_EXECUTABLE
             NAMES echo
             PATHS ${ECHO_EXECUTABLE_DIR}
             DOC "path to the echo executable")

message(STATUS "echo path: ${ECHO_EXECUTABLE}")

if("${ECHO_EXECUTABLE}" STREQUAL "ECHO_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "echo should be installed!")
endif()
add_definitions(-DECHO_EXEC=${ECHO_EXECUTABLE})
