cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.16)

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

if (WIN32)
  FetchContent_Declare(
    wasiclang
    URL
      https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-mingw.tar.gz
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  FetchContent_Declare(
    wasiclang
    URL
      https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-macos.tar.gz
    )
else()
  FetchContent_Declare(
    wasiclang
    URL
      https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-linux.tar.gz
    )
endif()
FetchContent_GetProperties(wasiclang)
if(NOT wasiclang_POPULATED)
  FetchContent_Populate(wasiclang)
endif()

FetchContent_Declare(
  mod_wasm2c
  GIT_REPOSITORY https://github.com/PLSysSec/wasm2c_sandbox_compiler
  GIT_TAG main)
FetchContent_GetProperties(mod_wasm2c)
if(NOT mod_wasm2c_POPULATED)
  FetchContent_Populate(mod_wasm2c)
endif()

# set (mod_wasm2c_SOURCE_DIR "/home/shr/Code/LibrarySandboxing/wasm2c_sandbox_compiler/")
# set (mod_wasm2c_SOURCE_DIR "C:/Users/shr/Code/wasm2c_sandbox_compiler/")

if(DEV)
  if(MSVC)
    set(RLBOX_SANITIZER_COMPILE_FLAGS)
    set(RLBOX_SANITIZER_LINK_FLAGS "")
  else()
    set(RLBOX_SANITIZER_COMPILE_FLAGS -fsanitize=address -fsanitize=undefined)
    set(RLBOX_SANITIZER_LINK_FLAGS -fsanitize=address -fsanitize=undefined)
  endif()
else()
  set(RLBOX_SANITIZER_COMPILE_FLAGS "")
  set(RLBOX_SANITIZER_LINK_FLAGS "")
endif()

if(DEV)
  set (WASM2C_BUILD_TYPE Debug)
else()
  if ("${DCMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (WASM2C_BUILD_TYPE Debug)
  elseif ("${DCMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set (WASM2C_BUILD_TYPE RelWithDebInfo)
  elseif ("${DCMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set (WASM2C_BUILD_TYPE MinSizeRel)
  else()
    set (WASM2C_BUILD_TYPE Release)
  endif()
endif()

# wasm2c ###################

set(WASM2C_RUNTIME_SOURCE_DIR "${mod_wasm2c_SOURCE_DIR}/wasm2c/")
set(WASM2C_RUNTIME_DIR "${mod_wasm2c_SOURCE_DIR}/build/")
set(WASM2C_COMPILER_DIR "${mod_wasm2c_SOURCE_DIR}/bin/")

set(C_SOURCE_FILES "${CMAKE_SOURCE_DIR}/c_src/wasm2c_sandbox_wrapper.c")
set(GLUE_LIB_WASM_DIR "${CMAKE_BINARY_DIR}/wasm/")
set(GLUE_LIB_WASM "${GLUE_LIB_WASM_DIR}/glue_lib_wasm2c.wasm")
set(GLUE_LIB_H "${GLUE_LIB_WASM_DIR}/glue_lib_wasm2c.h")
set(GLUE_LIB_C "${GLUE_LIB_WASM_DIR}/glue_lib_wasm2c.c")

add_custom_command(OUTPUT "${GLUE_LIB_H}" "${GLUE_LIB_C}" "${GLUE_LIB_WASM}"
                   DEPENDS ${C_SOURCE_FILES}
                   WORKING_DIRECTORY "${mod_wasm2c_SOURCE_DIR}"
                   COMMAND ${CMAKE_COMMAND}
                           -DCMAKE_BUILD_TYPE=${WASM2C_BUILD_TYPE}
                           -DCMAKE_C_FLAGS="${RLBOX_SANITIZER_COMPILE_FLAGS}"
                           -DCMAKE_CXX_FLAGS="${RLBOX_SANITIZER_COMPILE_FLAGS}"
                           -DLINK_FLAGS="${RLBOX_SANITIZER_LINK_FLAGS}"
                           -S  .
                           -B ./build
                   COMMAND ${CMAKE_COMMAND}
                           --build ./build
                           --config ${WASM2C_BUILD_TYPE}
                           --parallel
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLUE_LIB_WASM_DIR}
                    COMMAND ${wasiclang_SOURCE_DIR}/bin/clang
                            --sysroot ${wasiclang_SOURCE_DIR}/share/wasi-sysroot/
                            -O3
                            -Wl,--export-all -Wl,--no-entry -Wl,--growable-table -Wl,--stack-first -Wl,-z,stack-size=1048576
                            -o ${GLUE_LIB_WASM}
                            ${CMAKE_SOURCE_DIR}/c_src/wasm2c_sandbox_wrapper.c
                            ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib/libtest.c
                    COMMAND ${WASM2C_COMPILER_DIR}/wasm2c
                            -o ${GLUE_LIB_C}
                            ${GLUE_LIB_WASM}
                   COMMENT "Building wasm2c compiler, runtime and wasm sandboxed library")

# Tests ###################

if(DEV)
  add_compile_options(-Wall -Wextra -pedantic ${RLBOX_SANITIZER_COMPILE_FLAGS})
  add_link_options(${RLBOX_SANITIZER_LINK_FLAGS})
endif()

file(GLOB_RECURSE
     WASM2C_RUNTIME_CODE_C
     ${WASM2C_RUNTIME_SOURCE_DIR}/*.c)

set(WASM2C_RUNTIME_CODE ${WASM2C_RUNTIME_CODE_C} ${WASM2C_RUNTIME_SOURCE_DIR}/wasm-rt-shadow.cpp)
list(FILTER WASM2C_RUNTIME_CODE EXCLUDE REGEX "${WASM2C_RUNTIME_SOURCE_DIR}/examples/.*" )

add_library(glue_lib_static STATIC ${GLUE_LIB_C} ${WASM2C_RUNTIME_CODE})
target_include_directories(glue_lib_static PRIVATE ${mod_wasm2c_SOURCE_DIR}/wasm2c)

add_library(glue_lib_so SHARED ${GLUE_LIB_C} ${WASM2C_RUNTIME_CODE})
target_include_directories(glue_lib_so PRIVATE ${mod_wasm2c_SOURCE_DIR}/wasm2c)
# Add a dependency to glue_lib_static so we don't invoke the generator for GLUE_LIB_C multiple times
add_dependencies(glue_lib_so glue_lib_static)
# default firefox flags
# target_compile_options(glue_lib_so PRIVATE -Werror -Wall -Wempty-body -Wignored-qualifiers -Wpointer-arith -Wsign-compare -Wtype-limits -Wunreachable-code -Wduplicated-cond -Wno-error=maybe-uninitialized -Wno-error=deprecated-declarations -Wno-error=array-bounds -Wno-error=coverage-mismatch -Wno-error=free-nonheap-object -Wno-multistatement-macros -Wno-error=class-memaccess -Wno-error=deprecated-copy -Wno-error=unused-but-set-variable -Wformat -Wformat-overflow=2 -Wno-psabi -Wno-error=unused-function)

add_library(glue_lib_so_shadow_asan SHARED ${GLUE_LIB_C} ${WASM2C_RUNTIME_CODE})
target_include_directories(glue_lib_so_shadow_asan PRIVATE ${mod_wasm2c_SOURCE_DIR}/wasm2c)
target_compile_definitions(glue_lib_so_shadow_asan PRIVATE WASM_CHECK_SHADOW_MEMORY)
# Needed else both binaries invoke the custom command to generate ${GLUE_LIB_C}
add_dependencies(glue_lib_so_shadow_asan glue_lib_so)

include(CTest)
include(Catch)

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
target_link_libraries(test_rlbox_glue Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
)

target_compile_definitions(test_rlbox_glue PUBLIC GLUE_LIB_WASM2C_PATH="$<TARGET_FILE:glue_lib_so>")
add_dependencies(test_rlbox_glue glue_lib_so)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue rt)
endif()
catch_discover_tests(test_rlbox_glue)

####

add_executable(test_rlbox_glue_static test/test_wasm2c_sandbox_glue_main.cpp
                               test/test_wasm2c_sandbox_glue_static.cpp)
target_include_directories(test_rlbox_glue_static PUBLIC ${CMAKE_SOURCE_DIR}/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                           PUBLIC ${rlbox_SOURCE_DIR}/wasm
                                           PUBLIC ${mod_wasm2c_SOURCE_DIR}/wasm2c
                                           PUBLIC ${GLUE_LIB_WASM_DIR}
                                           )
target_link_libraries(test_rlbox_glue_static Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_static
)

add_dependencies(test_rlbox_glue_static glue_lib_so)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_static rt)
endif()
catch_discover_tests(test_rlbox_glue_static)

####

add_executable(test_rlbox_glue_smallheap test/test_wasm2c_sandbox_glue_main.cpp
                               test/test_wasm2c_sandbox_glue_smallheap.cpp)
target_include_directories(test_rlbox_glue_smallheap PUBLIC ${CMAKE_SOURCE_DIR}/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                           PUBLIC ${rlbox_SOURCE_DIR}/wasm
                                           PUBLIC ${mod_wasm2c_SOURCE_DIR}/wasm2c
                                           PUBLIC ${GLUE_LIB_WASM_DIR}
                                           )
target_link_libraries(test_rlbox_glue_smallheap Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_static
)

add_dependencies(test_rlbox_glue_smallheap glue_lib_so)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_smallheap rt)
endif()
catch_discover_tests(test_rlbox_glue_smallheap)

####

add_executable(test_rlbox_glue_embed test/test_wasm2c_sandbox_glue_main.cpp
                                     test/test_wasm2c_sandbox_glue_embedder_vars.cpp)
target_include_directories(test_rlbox_glue_embed PUBLIC ${CMAKE_SOURCE_DIR}/include
                                                 PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                                 PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                                 PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                                 PUBLIC ${rlbox_SOURCE_DIR}/wasm
                                                 PUBLIC ${mod_wasm2c_SOURCE_DIR}/wasm2c
                                                 )
target_link_libraries(test_rlbox_glue_embed Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
)

target_compile_definitions(test_rlbox_glue_embed PUBLIC GLUE_LIB_WASM2C_PATH="$<TARGET_FILE:glue_lib_so>")
add_dependencies(test_rlbox_glue_embed glue_lib_so)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_embed rt)
endif()
catch_discover_tests(test_rlbox_glue_embed)

####

add_executable(test_rlbox_glue_shadow_asan test/test_wasm2c_sandbox_glue_main.cpp
                                           test/test_wasm2c_sandbox_glue_shadow.cpp)
target_include_directories(test_rlbox_glue_shadow_asan PUBLIC ${CMAKE_SOURCE_DIR}/include
                                                       PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                                       PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                                       PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                                       PUBLIC ${rlbox_SOURCE_DIR}/wasm
                                                       PUBLIC ${mod_wasm2c_SOURCE_DIR}/wasm2c
                          )
target_link_libraries(test_rlbox_glue_shadow_asan Catch2::Catch2
                                                  ${CMAKE_THREAD_LIBS_INIT}
                                                  ${CMAKE_DL_LIBS}
)

target_compile_definitions(test_rlbox_glue_shadow_asan PUBLIC GLUE_LIB_WASM2C_PATH="$<TARGET_FILE:glue_lib_so_shadow_asan>")
add_dependencies(test_rlbox_glue_shadow_asan glue_lib_so_shadow_asan)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_shadow_asan rt)
endif()
catch_discover_tests(test_rlbox_glue_shadow_asan)

# Shortcuts ###################

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)
add_dependencies(check test_rlbox_glue)
add_dependencies(check test_rlbox_glue_static)
add_dependencies(check test_rlbox_glue_smallheap)
add_dependencies(check test_rlbox_glue_embed)
add_dependencies(check test_rlbox_glue_shadow_asan)
add_dependencies(check glue_lib_so)
add_dependencies(check glue_lib_so_shadow_asan)
