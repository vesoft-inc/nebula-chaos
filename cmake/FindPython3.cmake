# Copyright (c) 2020 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#

find_program(PYTHON3_EXECUTABLE
             NAMES python3
             PATHS ${PYTHON3_EXECUTABLE_DIR}
             DOC "path to the python3 executable")

message(STATUS "python3: ${PYTHON3_EXECUTABLE}")

if("${PYTHON3_EXECUTABLE}" STREQUAL "PYTHON3_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "python3 should be installed!")
endif()
add_definitions(-DPYTHON3_EXEC=${PYTHON3_EXECUTABLE})
