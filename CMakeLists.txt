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
        ast_printer.h)
