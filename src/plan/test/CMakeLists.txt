set(chaos_test_deps
    $<TARGET_OBJECTS:common_base_obj>
    $<TARGET_OBJECTS:common_datatypes_obj>
)

nebula_add_test(
    NAME
        nebula_instance_test
    SOURCES
        NebulaInstanceTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:nebula_instance_obj>
        $<TARGET_OBJECTS:ssh_helper_obj>
        ${chaos_test_deps}
    LIBRARIES
        gtest
)

