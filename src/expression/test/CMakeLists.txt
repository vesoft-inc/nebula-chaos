
nebula_add_test(
    NAME
        expr_test
    SOURCES
        ExprTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:common_base_obj>
        $<TARGET_OBJECTS:expr_obj>
    LIBRARIES
        gtest
)

