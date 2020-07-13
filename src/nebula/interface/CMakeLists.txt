
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

set(THRIFT1 ${Fbthrift_BIN})
set(THRIFT_TEMPLATES ${Fbthrift_INCLUDE_DIR}/thrift/templates)
include(ThriftGenerate)

# Target object name : graph_thrift_obj
thrift_generate("graph" "GraphService" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} "nebula/interface")

add_custom_target(
    clean-interface
    COMMAND "rm" "-fr" "gen-cpp2" "gen-java" "gen-go" "gen-py"
)
