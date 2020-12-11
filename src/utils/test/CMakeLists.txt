set(chaos_test_deps
    $<TARGET_OBJECTS:common_base_obj>
)

nebula_add_test(
    NAME
        sub_process_test
    SOURCES
        SubProcessTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:ssh_helper_obj>
        ${chaos_test_deps}
    LIBRARIES
        gtest
)

nebula_add_test(
    NAME
        ssh_helper_test
    SOURCES
        SshHelperTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:ssh_helper_obj>
        ${chaos_test_deps}
    LIBRARIES
        gtest
)
