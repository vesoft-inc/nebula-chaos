include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(BISON 3.0.5 REQUIRED)
find_package(FLEX REQUIRED)

bison_target(Parser parser.yy ${CMAKE_CURRENT_BINARY_DIR}/ExprParser.cpp COMPILE_FLAGS "-Werror")
flex_target(Scanner scanner.lex ${CMAKE_CURRENT_BINARY_DIR}/ExprScanner.cpp)

add_custom_target(parser_target DEPENDS ${FLEX_Scanner_OUTPUTS} ${BISON_Parser_OUTPUTS})

add_flex_bison_dependency(Scanner Parser)

add_compile_options(-Wno-sign-compare -Wno-conversion-null -Wno-pedantic -Wno-extra)

message(${CMAKE_CURRENT_BINARY_DIR})
message(${FLEX_Scanner_OUTPUTS})
message(${BISON_Parser_OUTPUTS})

nebula_add_library(
    parser_obj OBJECT
    ${FLEX_Scanner_OUTPUTS}
    ${BISON_Parser_OUTPUTS}
)

add_subdirectory(test)
