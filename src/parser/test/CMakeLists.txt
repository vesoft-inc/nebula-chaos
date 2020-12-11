set(chaos_test_deps
    $<TARGET_OBJECTS:common_base_obj>
    $<TARGET_OBJECTS:common_datatypes_obj>
)

nebula_add_test(
    NAME
        parser_test
    SOURCES
        ParserTest.cpp
    OBJECTS
        ${chaos_test_deps}
        $<TARGET_OBJECTS:expr_obj>
        $<TARGET_OBJECTS:parser_obj>
    LIBRARIES
        gtest
)

