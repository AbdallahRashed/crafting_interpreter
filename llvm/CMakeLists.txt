cmake_minimum_required(VERSION 3.20)
project(lox-llvm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG
    PATHS /usr/lib/llvm-20/cmake)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(lox-llvm
    src/main.cpp
    src/scanner.cpp
    src/parser.cpp
    src/codegen.cpp
)

llvm_map_components_to_libnames(llvm_libs
    core
    executionengine
    interpreter
    mc
    mcjit
    support
    nativecodegen
    orcjit
    target
    x86asmparser
    x86codegen
    x86desc
    x86info
    passes
)

target_link_libraries(lox-llvm ${llvm_libs})
target_include_directories(lox-llvm PRIVATE ${CMAKE_SOURCE_DIR}/src)
