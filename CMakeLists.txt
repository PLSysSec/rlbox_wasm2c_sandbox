cmake_minimum_required(VERSION 3.13)

project(rlbox_wasm2c
        VERSION 0.1
        DESCRIPTION "RLBox integration with WASM modules compiled with wasm2c")

# Project Settings ###################

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEV "Use settings suitable for dev contributions to rlbox" OFF)

file(GLOB_RECURSE
     ALL_CXX_SOURCE_FILES
     ${CMAKE_SOURCE_DIR}/include/*.[chi]pp
     ${CMAKE_SOURCE_DIR}/include/*.[chi]xx
     ${CMAKE_SOURCE_DIR}/include/*.cc
     ${CMAKE_SOURCE_DIR}/include/*.hh
     ${CMAKE_SOURCE_DIR}/include/*.ii
     ${CMAKE_SOURCE_DIR}/include/*.[CHI]
     ${CMAKE_SOURCE_DIR}/test/*.[chi]pp
     ${CMAKE_SOURCE_DIR}/test/*.[chi]xx
     ${CMAKE_SOURCE_DIR}/test/*.cc
     ${CMAKE_SOURCE_DIR}/test/*.hh
     ${CMAKE_SOURCE_DIR}/test/*.ii
     ${CMAKE_SOURCE_DIR}/test/*.[CHI]
     ${CMAKE_SOURCE_DIR}/c_src/*.[chi]pp
     ${CMAKE_SOURCE_DIR}/c_src/*.[chi]xx
     ${CMAKE_SOURCE_DIR}/c_src/*.cc
     ${CMAKE_SOURCE_DIR}/c_src/*.hh
     ${CMAKE_SOURCE_DIR}/c_src/*.ii
     ${CMAKE_SOURCE_DIR}/c_src/*.[CHI])

# Dev Tools ###################

if(DEV)
  find_program(CLANG_TIDY "clang-tidy")
  if(CLANG_TIDY)
    # Config in .clang-tidy
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
  endif()

  find_program(CLANG_FORMAT "clang-format")
  if(CLANG_FORMAT)
    # Config in .clang-format
    add_custom_target(format-source
                      COMMAND clang-format
                              -i
                              -style=file
                              ${ALL_CXX_SOURCE_FILES})
  endif()

endif()

# Dependencies ###################

find_program(CARGO "cargo")
if(!CARGO)
  message(
    FATAL_ERROR
      "Could not find cargo. Please install cargo as it is needed to build rust libraries."
    )
endif()

find_program(EMCC "emcc")
if(!EMCC)
  message(
    FATAL_ERROR
      "Could not find emcc. Please install emcc as it is needed to generate the wasm2c sandbox."
    )
endif()

include(FetchContent)

FetchContent_Declare(
  rlbox
  GIT_REPOSITORY https://github.com/PLSysSec/rlbox_api_cpp17.git)
FetchContent_GetProperties(rlbox)
if(NOT rlbox_POPULATED)
  FetchContent_Populate(rlbox)
endif()

FetchContent_Declare(catch2
                     GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                     GIT_TAG v2.9.1)
FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  FetchContent_Populate(catch2)
endif()

add_subdirectory("${catch2_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/contrib")

FetchContent_Declare(
  mod_wasm2c
  GIT_REPOSITORY https://github.com/PLSysSec/wasm2c_sandbox_compiler
  GIT_TAG main)
FetchContent_GetProperties(mod_wasm2c)
if(NOT mod_wasm2c_POPULATED)
  FetchContent_Populate(mod_wasm2c)
endif()

# wasm2c ###################

set(WASM2C_RUNTIME_DIR "${mod_wasm2c_SOURCE_DIR}/build/")
set(WASM2C_COMPILER_DIR "${mod_wasm2c_SOURCE_DIR}/build/")
set(WASM2C_COMPILER_PATH "${WASM2C_COMPILER_DIR}/wasm2c")

add_custom_command(OUTPUT "${WASM2C_COMPILER_PATH}"
                   WORKING_DIRECTORY "${mod_wasm2c_SOURCE_DIR}"
                   COMMAND ${CMAKE_COMMAND}
                           -DCMAKE_BUILD_TYPE=Release
                           -S
                           .
                           -B
                           ./build
                   COMMAND VERBOSE=1
                           ${CMAKE_COMMAND}
                           --build
                           ./build
                           --parallel
                           --target
                           wasm2c
                   COMMAND VERBOSE=1
                           ${CMAKE_COMMAND}
                           --build
                           ./build
                           --parallel
                           --target
                           wasm-rt-impl
                   COMMENT "Building wasm2c compiler and runtime")

add_custom_target(wasm2c_release ALL DEPENDS ${WASM2C_COMPILER_PATH})

# Tests ###################

if(DEV)
  if(MSVC)
    add_compile_options(/W4) # warnings
    add_compile_options(/WX) # warnings as errors
  else()
    add_compile_options(-Wall -Wextra -pedantic) # warnings
    add_compile_options(-Werror) # warnings as errors
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

include(CTest)
include(Catch)

set(GLUE_LIB_WASM "${CMAKE_BINARY_DIR}/wasm/glue_lib_wasm2c.wasm")
set(GLUE_LIB_C "${CMAKE_BINARY_DIR}/wasm/glue_lib_wasm2c.c")

add_custom_command(OUTPUT ${GLUE_LIB_WASM} ${GLUE_LIB_C}
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/c_src
                   COMMAND rlbox_SOURCE_DIR=${rlbox_SOURCE_DIR}
                           WASM2C_COMPILER_DIR="${WASM2C_COMPILER_DIR}"
                           ${CMAKE_COMMAND}
                           -S
                           .
                           -B
                           ${CMAKE_BINARY_DIR}/wasm
                   COMMAND VERBOSE=1
                           ${CMAKE_COMMAND}
                           --build
                           ${CMAKE_BINARY_DIR}/wasm
                           --target
                           all
                   COMMENT "Building wasm library")

add_custom_target(glue_lib_wasm ALL DEPENDS wasm2c_release ${GLUE_LIB_WASM} ${GLUE_LIB_C})

add_library(glue_lib_so SHARED ${CMAKE_BINARY_DIR}/wasm/glue_lib_wasm2c.c)
target_include_directories(glue_lib_so PRIVATE ${mod_wasm2c_SOURCE_DIR}/wasm2c)

find_package(Threads REQUIRED)

# Tests executables ###################

add_executable(test_rlbox_glue test/test_wasm2c_sandbox_glue_main.cpp
                               test/test_wasm2c_sandbox_glue.cpp)
target_include_directories(test_rlbox_glue PUBLIC ${CMAKE_SOURCE_DIR}/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                           PUBLIC ${rlbox_SOURCE_DIR}/wasm
                                           PUBLIC ${mod_wasm2c_SOURCE_DIR}/wasm2c
                                           )
target_link_directories(test_rlbox_glue PUBLIC ${WASM2C_RUNTIME_DIR})
target_link_libraries(test_rlbox_glue Catch2::Catch2
                                      wasm-rt-impl
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
)

target_compile_definitions(test_rlbox_glue PUBLIC GLUE_LIB_WASM2C_PATH="$<TARGET_FILE:glue_lib_so>")
add_dependencies(test_rlbox_glue glue_lib_so)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue rt)
endif()
catch_discover_tests(test_rlbox_glue)

# ####

# add_executable(test_rlbox_glue_embed test/test_wasm2c_sandbox_glue_main.cpp
#                                      test/test_wasm2c_sandbox_glue_embedder_vars.cpp)
# target_include_directories(test_rlbox_glue_embed PUBLIC include)
# target_include_directories(test_rlbox_glue_embed
#                            PUBLIC ${rlbox_SOURCE_DIR}/code/include)
# target_include_directories(test_rlbox_glue_embed
#                            PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue)
# target_include_directories(test_rlbox_glue_embed
#                            PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib)

# target_compile_definitions(test_rlbox_glue_embed PUBLIC
#                            GLUE_LIB_WASM2C_PATH="${GLUE_LIB_SO}")

# target_link_libraries(test_rlbox_glue_embed
#                       Catch2::Catch2
#                       ${LINK_WASM2C_LIB}
#                       ${CMAKE_THREAD_LIBS_INIT}
#                       ${CMAKE_DL_LIBS}
#                       wasm2c_sandbox
#                       )

# if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
#   target_link_libraries(test_rlbox_glue_embed rt)
# endif()
# catch_discover_tests(test_rlbox_glue_embed)

# # Shortcuts ###################

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)
add_dependencies(check test_rlbox_glue # test_rlbox_glue_embed
 glue_lib_so)
