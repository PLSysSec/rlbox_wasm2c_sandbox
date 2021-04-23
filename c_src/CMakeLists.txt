cmake_minimum_required(VERSION 3.13)

# Mac adds extra flags
set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)

project(rlbox_wasm2c_testlib
        VERSION 0.1
        DESCRIPTION "RLBox integration with WASM modules compiled with wasm2c")

if(NOT DEFINED ENV{rlbox_SOURCE_DIR})
  message(FATAL_ERROR "Set rlbox_SOURCE_DIR environment variable")
endif()

if(NOT DEFINED ENV{WASM2C_COMPILER_DIR})
  message(FATAL_ERROR "Set WASM2C_COMPILER_DIR environment variable")
endif()

set(rlbox_SOURCE_DIR $ENV{rlbox_SOURCE_DIR})
set(WASM2C_COMPILER_DIR $ENV{WASM2C_COMPILER_DIR})

set(CMAKE_C_COMPILER emcc)
set(CMAKE_CXX_COMPILER em++)
set(CMAKE_BUILD_TYPE Release)

# Link flags are set by default on Mac - clearing this
set(CMAKE_C_LINK_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--export-all --no-entry")

add_executable(glue_lib_wasm2c.wasm
               ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib/libtest.c)

set(GLUE_LIB_PATH "${CMAKE_BINARY_DIR}/glue_lib_wasm2c.c")

add_custom_command(OUTPUT ${GLUE_LIB_PATH}
                   DEPENDS glue_lib_wasm2c.wasm
                   COMMAND ${WASM2C_COMPILER_DIR}/wasm2c
                           -o
                           ${GLUE_LIB_PATH}
                           glue_lib_wasm2c.wasm
                   COMMENT "Compiling wasm file to C with checks")

add_custom_target(glue_lib_c ALL DEPENDS ${GLUE_LIB_PATH})
