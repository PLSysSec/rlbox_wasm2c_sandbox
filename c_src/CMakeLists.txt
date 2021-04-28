cmake_minimum_required(VERSION 3.13)

# Mac adds extra flags
set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)

project(rlbox_wasm2c_testlib
        VERSION 0.1
        DESCRIPTION "RLBox integration with WASM modules compiled with wasm2c")

if(NOT DEFINED ENV{rlbox_SOURCE_DIR})
  message(FATAL_ERROR "Set rlbox_SOURCE_DIR environment variable")
endif()

if(NOT DEFINED ENV{wasiclang_SOURCE_DIR})
  message(FATAL_ERROR "Set wasiclang_SOURCE_DIR environment variable")
endif()

if(NOT DEFINED ENV{WASM2C_COMPILER_DIR})
  message(FATAL_ERROR "Set WASM2C_COMPILER_DIR environment variable")
endif()

set(rlbox_SOURCE_DIR $ENV{rlbox_SOURCE_DIR})
set(wasiclang_SOURCE_DIR $ENV{wasiclang_SOURCE_DIR})
set(WASM2C_COMPILER_DIR $ENV{WASM2C_COMPILER_DIR})

set(CMAKE_BUILD_TYPE Release)
#####################################################
# Dynamic loaded sandboxed library flags
set(CMAKE_C_COMPILER ${wasiclang_SOURCE_DIR}/bin/clang)
set(CMAKE_CXX_COMPILER ${wasiclang_SOURCE_DIR}/bin/clang++)

# Apply settings suitable for wasm module compilation
set(CMAKE_C_FLAGS
    "--sysroot ${wasiclang_SOURCE_DIR}/share/wasi-sysroot/")
# Link flags are set by default on Mac - clearing this
set(CMAKE_C_LINK_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "-v -Wl,--export-all -Wl,--no-entry -Wl,--growable-table")
#####################################################
# Static linked sandboxed library flags
# set(CMAKE_C_COMPILER /mnt/sata/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang)
# set(CMAKE_CXX_COMPILER /mnt/sata/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang++)

# # Apply settings suitable for wasm module compilation
# set(CMAKE_C_FLAGS
#     "--sysroot ${wasiclang_SOURCE_DIR}/share/wasi-sysroot/ --target=wasm32-unknown-wasi")
# # Link flags are set by default on Mac - clearing this
# set(CMAKE_C_LINK_FLAGS "-nostdlib")
# set(CMAKE_EXE_LINKER_FLAGS "-Wl,--export-all -Wl,--no-entry -Wl,--growable-table -Wl,--allow-undefined")
#####################################################

add_executable(glue_lib_wasm2c.wasm
               wasm2c_sandbox_wrapper.c
               ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib/libtest.c)

set(GLUE_LIB_PATH "${CMAKE_BINARY_DIR}/glue_lib_wasm2c.c")

add_custom_command(OUTPUT ${GLUE_LIB_PATH}
                   DEPENDS glue_lib_wasm2c.wasm
                   COMMAND ${WASM2C_COMPILER_DIR}/wasm2c
                           -o ${GLUE_LIB_PATH}
                           -n "currlib_"
                           glue_lib_wasm2c.wasm
                   COMMENT "Compiling wasm file to C with checks")

add_custom_target(glue_lib_c ALL DEPENDS ${GLUE_LIB_PATH})
