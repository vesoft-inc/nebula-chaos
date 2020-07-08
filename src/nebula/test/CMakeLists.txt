nebula_add_test(
    NAME
        nebula_instance_test
    SOURCES
        NebulaInstanceTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:nebula_instance_obj>
        $<TARGET_OBJECTS:ssh_helper_obj>
        $<TARGET_OBJECTS:base_obj>
    LIBRARIES
        gtest
)

