# Copyright (c) 2020 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#

find_program(SSH_EXECUTABLE
             NAMES ssh
             PATHS ${SSH_EXECUTABLE_DIR}
             DOC "path to the ssh executable")

message(STATUS "ssh path: ${SSH_EXECUTABLE}")

if("${SSH_EXECUTABLE}" STREQUAL "SSH_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "ssh should be installed!")
endif()
add_definitions(-DSSH_EXEC=${SSH_EXECUTABLE})
