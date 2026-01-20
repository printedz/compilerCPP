cmake_minimum_required(VERSION 3.20)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

add_executable(compiler 
        main.cpp 
        lexer.cpp 
        parser.cpp 
        codegen.cpp 
        ast_printer.cpp 
        resolver.cpp
        ast.h 
        lexer.h 
        parser.h 
        codegen.h 
        ast_printer.h
        resolver.h
        ir.h
        ir_printer.cpp
        ir_printer.h
        lowering.cpp
        lowering.h)

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
