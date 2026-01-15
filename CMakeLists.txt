cmake_minimum_required(VERSION 3.20)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

add_executable(compiler 
        main.cpp 
        lexer.cpp 
        parser.cpp 
        codegen.cpp 
        ast_printer.cpp 
        ast.h 
        lexer.h 
        parser.h 
        codegen.h 
        ast_printer.h
        ir.h
        ir_printer.cpp
        ir_printer.h
        lowering.cpp
        lowering.h)

add_custom_target(lex
    COMMAND $<TARGET_FILE:compiler> --lex ${CMAKE_SOURCE_DIR}/test/test.c
    DEPENDS compiler)

add_custom_target(parse
    COMMAND $<TARGET_FILE:compiler> --parse ${CMAKE_SOURCE_DIR}/test/test.c
    DEPENDS compiler)

add_custom_target(ir
    COMMAND $<TARGET_FILE:compiler> --ir ${CMAKE_SOURCE_DIR}/test/test.c
    DEPENDS compiler)

add_custom_target(codegen
    COMMAND $<TARGET_FILE:compiler> --codegen ${CMAKE_SOURCE_DIR}/test/test.c
    DEPENDS compiler)
