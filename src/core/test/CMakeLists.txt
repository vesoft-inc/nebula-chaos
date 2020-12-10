set(chaos_test_deps
    $<TARGET_OBJECTS:common_base_obj>
    $<TARGET_OBJECTS:common_datatypes_obj>
)

nebula_add_test(
    NAME
        actions_test
    SOURCES
        ActionsTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:actions_obj>
        $<TARGET_OBJECTS:parser_obj>
        $<TARGET_OBJECTS:expr_obj>
        $<TARGET_OBJECTS:ssh_helper_obj>
        ${chaos_test_deps}
    LIBRARIES
        gtest
)

