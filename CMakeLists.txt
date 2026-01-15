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

add_executable(run_lex main.cpp)
set_target_properties(run_lex PROPERTIES OUTPUT_NAME "run-lex")
add_custom_command(TARGET run_lex POST_BUILD
    COMMAND $<TARGET_FILE:compiler> --lex ${CMAKE_SOURCE_DIR}/test/test.c
)

add_executable(run_parse main.cpp)
set_target_properties(run_parse PROPERTIES OUTPUT_NAME "run-parse")
add_custom_command(TARGET run_parse POST_BUILD
    COMMAND $<TARGET_FILE:compiler> --parse ${CMAKE_SOURCE_DIR}/test/test.c
)

add_executable(run_ir main.cpp)
set_target_properties(run_ir PROPERTIES OUTPUT_NAME "run-ir")
add_custom_command(TARGET run_ir POST_BUILD
    COMMAND $<TARGET_FILE:compiler> --ir ${CMAKE_SOURCE_DIR}/test/test.c
)

add_executable(run_codegen main.cpp)
set_target_properties(run_codegen PROPERTIES OUTPUT_NAME "run-codegen")
add_custom_command(TARGET run_codegen POST_BUILD
    COMMAND $<TARGET_FILE:compiler> --codegen ${CMAKE_SOURCE_DIR}/test/test.c
)