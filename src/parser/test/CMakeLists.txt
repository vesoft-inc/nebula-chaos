
nebula_add_test(
    NAME
        parser_test
    SOURCES
        ParserTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:common_base_obj>
        $<TARGET_OBJECTS:expr_obj>
        $<TARGET_OBJECTS:parser_obj>
    LIBRARIES
        gtest
)

