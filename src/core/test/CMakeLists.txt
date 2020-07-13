
nebula_add_test(
    NAME
        actions_test
    SOURCES
        ActionsTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:actions_obj>
        $<TARGET_OBJECTS:ssh_helper_obj>
        $<TARGET_OBJECTS:base_obj>
    LIBRARIES
        gtest
)

