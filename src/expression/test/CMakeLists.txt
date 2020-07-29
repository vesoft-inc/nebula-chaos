
nebula_add_test(
    NAME
        expr_test
    SOURCES
        ExprTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:expr_obj>
        $<TARGET_OBJECTS:base_obj>
    LIBRARIES
        gtest
)

