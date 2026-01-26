cmake_minimum_required(VERSION 3.20)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

set(COMPILER_SOURCES
        lexer.cpp
        parser.cpp
        codegen.cpp
        ast_printer.cpp
        resolver.cpp
        ir_printer.cpp
        lowering.cpp)

add_library(compiler_lib ${COMPILER_SOURCES})
target_include_directories(compiler_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(compiler main.cpp)
target_link_libraries(compiler PRIVATE compiler_lib)

include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(compiler_tests
        tests/lexer_tests.cpp
        tests/parser_tests.cpp
        tests/resolver_tests.cpp)
target_link_libraries(compiler_tests PRIVATE compiler_lib GTest::gtest_main)
add_test(NAME compiler_tests COMMAND compiler_tests)

function(add_compiler_action_target target_name action_arg)
    add_custom_target(${target_name}
        COMMAND $<TARGET_FILE:compiler> ${action_arg} ${CMAKE_SOURCE_DIR}/test/test.c
        DEPENDS compiler)

    set_target_properties(${target_name} PROPERTIES
        XCODE_GENERATE_SCHEME TRUE
        XCODE_SCHEME_EXECUTABLE "${CMAKE_BINARY_DIR}/Debug/compiler"
        XCODE_SCHEME_LAUNCH_CONFIGURATION Debug
        XCODE_SCHEME_ARGUMENTS "${action_arg};${CMAKE_SOURCE_DIR}/test/test.c"
        XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

add_compiler_action_target(lex --lex)
add_compiler_action_target(parse --parse)
add_compiler_action_target(ir --ir)
add_compiler_action_target(codegen --codegen)
