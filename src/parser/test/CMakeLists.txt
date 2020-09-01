
nebula_add_test(
    NAME
        parser_test
    SOURCES
        ParserTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:expr_obj>
        $<TARGET_OBJECTS:parser_obj>
        $<TARGET_OBJECTS:base_obj>
    LIBRARIES
        gtest
)

