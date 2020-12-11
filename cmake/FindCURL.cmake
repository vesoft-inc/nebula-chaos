# Copyright (c) 2020 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#

find_program(CURL_EXECUTABLE
             NAMES curl
             PATHS ${CURL_EXECUTABLE_DIR}
             DOC "path to the curl executable")

message(STATUS "curl path: ${CURL_EXECUTABLE}")

if("${CURL_EXECUTABLE}" STREQUAL "CURL_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "curl should be installed!")
endif()
add_definitions(-DCURL_EXEC=${CURL_EXECUTABLE})
