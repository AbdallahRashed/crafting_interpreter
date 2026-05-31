cmake_minimum_required(VERSION 3.20)
project(lox-mlir CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MLIR REQUIRED CONFIG
    PATHS /usr/lib/llvm-20/lib/cmake/mlir)
find_package(LLVM REQUIRED CONFIG
    PATHS /usr/lib/llvm-20/cmake)

message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Get needed LLVM libraries
llvm_map_components_to_libnames(llvm_libs
    core support nativecodegen target x86asmparser x86codegen x86desc x86info passes)

add_executable(lox-mlir
    src/main.cpp
    src/scanner.cpp
    src/parser.cpp
    src/mlirgen.cpp
)

target_include_directories(lox-mlir PRIVATE ${CMAKE_SOURCE_DIR}/src)

# MLIR libraries
get_property(mlir_dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(mlir_conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)

target_link_libraries(lox-mlir
    ${mlir_dialect_libs}
    ${mlir_conversion_libs}
    MLIROptLib
    MLIRExecutionEngine
    ${llvm_libs}
)
