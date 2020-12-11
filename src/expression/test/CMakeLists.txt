set(chaos_test_deps
    #$<TARGET_OBJECTS:client_nebula_graph_client_obj>
    $<TARGET_OBJECTS:common_base_obj>
    #$<TARGET_OBJECTS:common_common_thrift_obj>
    #$<TARGET_OBJECTS:common_graph_thrift_obj>
    $<TARGET_OBJECTS:common_datatypes_obj>
    #$<TARGET_OBJECTS:common_graph_obj>
)

nebula_add_test(
    NAME
        expr_test
    SOURCES
        ExprTest.cpp
    OBJECTS
        ${chaos_test_deps}
        $<TARGET_OBJECTS:expr_obj>
    LIBRARIES
        gtest
)

