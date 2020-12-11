# Copyright (c) 2018 vesoft inc. All rights reserved.
#
# This source code is licensed under Apache 2.0 License,
# attached with Common Clause Condition 1.0, found in the LICENSES directory.
#
# The build can be controlled by defining following variables on the
# <cmake> command line
#
#   CMAKE_C_COMPILER               -- Specify the compiler for C language
#   CMAKE_CXX_COMPILER             -- Specify the compiler for C++ language
#
#   NEBULA_CLIENT_SOURCE_DIR       -- Path to nebula-client source directory
#   NEBULA_CLIENT_BUILD_DIR        -- Path to nebula-client build directory
#   NEBULA_CLIENT_REPO_URL         -- Git URL for the nebula-client repo
#   NEBULA_CLIENT_REPO_TAG         -- Tag/branch of the nebula-client repo

#   NEBULA_COMMON_REPO_URL         -- Git URL for the nebula-common repo
#   NEBULA_COMMON_REPO_TAG         -- Tag/branch of the nebula-common repo
#
#   ENABLE_JEMALLOC                -- Link jemalloc into all executables
#   ENABLE_NATIVE                  -- Build native client
#   ENABLE_TESTING                 -- Build unit test
#

cmake_minimum_required(VERSION 3.5.0)

project("Nebula Chaos" C CXX)

option(ENABLE_CLIENT_MODULE_UPDATE "Automatically update client module" OFF)
option(ENABLE_COMMON_MODULE_UPDATE "Automatically update module" OFF)

# Set the project home dir
add_definitions(-DNEBULA_CHAOS_HOME=${CMAKE_SOURCE_DIR})

# Submodules cpp client
if("${NEBULA_CLIENT_REPO_URL}" STREQUAL "")
    SET(NEBULA_CLIENT_REPO_URL "https://github.com/vesoft-inc/nebula-cpp.git")
endif()

if("${NEBULA_CLIENT_REPO_TAG}" STREQUAL "")
    SET(NEBULA_CLIENT_REPO_TAG "master")
endif()

# Submodules common
if("${NEBULA_COMMON_REPO_URL}" STREQUAL "")
    SET(NEBULA_COMMON_REPO_URL "https://github.com/vesoft-inc/nebula-common.git")
endif()

if("${NEBULA_COMMON_REPO_TAG}" STREQUAL "")
    SET(NEBULA_COMMON_REPO_TAG "master")
endif()

# To include customized FindXXX.cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

if(NOT NEBULA_CLIENT_SOURCE_DIR)
    include(FetchModule)
    # fetch graph cpp client repo
    nebula_fetch_module(
        NAME
            client
        URL
            ${NEBULA_CLIENT_REPO_URL}
        TAG
            ${NEBULA_CLIENT_REPO_TAG}
        UPDATE
            ${ENABLE_CLIENT_MODULE_UPDATE}
    )
    set(nebula_client_source_dir ${CMAKE_SOURCE_DIR}/modules/client)
    set(nebula_client_build_dir ${CMAKE_BINARY_DIR}/modules/client)

    # fetch common repo
    nebula_fetch_module(
        NAME
            common
        URL
            ${NEBULA_COMMON_REPO_URL}
        TAG
            ${NEBULA_COMMON_REPO_TAG}
        UPDATE
            ${ENABLE_COMMON_MODULE_UPDATE}
    )
else()
    set(nebula_client_source_dir ${NEBULA_CLIENT_SOURCE_DIR})
    if(NOT NEBULA_CLIENT_BUILD_DIR)
        set(nebula_client_build_dir ${CMAKE_BINARY_DIR}/modules/client)
    else()
        set(nebula_client_build_dir ${NEBULA_CLIENT_BUILD_DIR})
    endif()
endif()

# graph cpp client cmake
list(APPEND CMAKE_MODULE_PATH ${nebula_client_source_dir}/cmake)
list(APPEND CMAKE_MODULE_PATH ${nebula_client_source_dir}/cmake/nebula)

# common cmake
list(APPEND CMAKE_MODULE_PATH ${nebula_client_source_dir}/modules/common/cmake)
list(APPEND CMAKE_MODULE_PATH ${nebula_client_source_dir}/modules/common/cmake/nebula)

include(PlatformCheck)
include(NebulaCMakeMacros)
include(GeneralCMakeOptions)
include(ABIConfig)
include(GeneralCMakeConfig)
include(GeneralCompilerConfig)
include(LinkerConfig)
include(CcacheConfig)
include(ThirdPartyConfig)
include(SanitizerConfig)
include(GitHooksConfig)
include(GitInfoConfig)
include(NebulaCustomTargets)

include(ConfigNebulaClient)

config_nebula_client(
    SOURCE_DIR ${nebula_client_source_dir}
    BUILD_DIR ${nebula_client_build_dir}
)

include(FindSSH)
include(FindMail)
include(FindEcho)
include(FindPython3)
include(FindCURL)

# For simplicity, we make all ordinary libraries depend on the compile-time generated files,
# including the precompiled header, a.k.a Base.h.gch, and thrift headers.
macro(nebula_add_library name type)
    add_library(${name} ${type} ${ARGN})
    add_dependencies(
        ${name}
        client_project
    )
endmacro()

include_directories(AFTER ${CMAKE_SOURCE_DIR}/modules/client/modules/common/src)
include_directories(AFTER ${CMAKE_SOURCE_DIR}/modules/client/include)
include_directories(AFTER ${CMAKE_SOURCE_DIR}/modules/client/src)
include_directories(AFTER ${CMAKE_SOURCE_DIR}/src)
include_directories(AFTER ${CMAKE_CURRENT_BINARY_DIR}/modules/client/modules/common/src)
include_directories(AFTER ${CMAKE_CURRENT_BINARY_DIR}/modules/client/src)
include_directories(AFTER ${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(AFTER ${CMAKE_CURRENT_BINARY_DIR}/src/parser)

add_subdirectory(src)

add_custom_target(
    clean-chaos-modules
    COMMAND "rm" "-fr" "modules/*"
)

add_custom_target(
    clean-chaos-build
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND "find" "." "-name" "Testing" "|" "xargs" "rm" "-fr"
)

add_custom_target(
    clean-chaos-all
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND "find" "." "-name" "Testing" "|" "xargs" "rm" "-fr"
    DEPENDS clean-chaos-modules
)

add_custom_target(
    distchaosclean
    COMMAND "find" "." "-name" "CMakeFiles" "|" "xargs" "rm" "-fr"
    COMMAND "find" "." "-name" "CMakeCache.txt" "|" "xargs" "rm" "-f"
    COMMAND "find" "." "-name" "cmake_install.cmake" "|" "xargs" "rm" "-f"
    COMMAND "find" "." "-name" "CTestTestfile.cmake" "|" "xargs" "rm" "-f"
    COMMAND "find" "." "-name" "CPackConfig.cmake" "|" "xargs" "rm" "-f"
    COMMAND "find" "." "-name" "CPackSourceConfig.cmake" "|" "xargs" "rm" "-f"
    COMMAND "find" "." "-name" "Makefile" "|" "xargs" "rm" "-f"
    DEPENDS clean-chaos-all
)

