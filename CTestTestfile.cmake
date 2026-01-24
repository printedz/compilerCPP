# CMake generated Testfile for 
# Source directory: C:/Users/Aleja/source/repos/compilerCPP
# Build directory: C:/Users/Aleja/source/repos/compilerCPP
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[compiler_tests]=] "C:/Users/Aleja/source/repos/compilerCPP/Debug/compiler_tests.exe")
  set_tests_properties([=[compiler_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;34;add_test;C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[compiler_tests]=] "C:/Users/Aleja/source/repos/compilerCPP/Release/compiler_tests.exe")
  set_tests_properties([=[compiler_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;34;add_test;C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[compiler_tests]=] "C:/Users/Aleja/source/repos/compilerCPP/MinSizeRel/compiler_tests.exe")
  set_tests_properties([=[compiler_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;34;add_test;C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[compiler_tests]=] "C:/Users/Aleja/source/repos/compilerCPP/RelWithDebInfo/compiler_tests.exe")
  set_tests_properties([=[compiler_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;34;add_test;C:/Users/Aleja/source/repos/compilerCPP/CMakeLists.txt;0;")
else()
  add_test([=[compiler_tests]=] NOT_AVAILABLE)
endif()
subdirs("_deps/googletest-build")
