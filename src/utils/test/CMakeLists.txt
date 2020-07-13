
nebula_add_test(
    NAME
        sub_process_test
    SOURCES
        SubProcessTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:base_obj>
    LIBRARIES
        gtest
)

nebula_add_test(
    NAME
        ssh_helper_test
    SOURCES
        SshHelperTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:base_obj>
        $<TARGET_OBJECTS:ssh_helper_obj>
    LIBRARIES
        gtest
)
