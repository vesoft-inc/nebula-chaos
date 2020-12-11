# Copyright (c) 2020 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#

find_program(MAIL_EXECUTABLE
             NAMES mail
             PATHS ${MAIL_EXECUTABLE_DIR}
             DOC "path to the mail executable")

message(STATUS "mail path: ${MAIL_EXECUTABLE}")

if("${MAIL_EXECUTABLE}" STREQUAL "MAIL_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "mail should be installed!")
endif()
add_definitions(-DMAIL_EXEC=${MAIL_EXECUTABLE})
