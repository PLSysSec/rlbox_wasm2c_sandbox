cmake_minimum_required(VERSION 3.30)

# TODO: Migrate from FetchContent_Populate
cmake_policy(SET CMP0169 OLD)

project(rlbox_wasm2c
        VERSION 0.1
        DESCRIPTION "RLBox integration with WASM modules compiled with wasm2c")

# Project Settings ###################

# set(CMAKE_VERBOSE_MAKEFILE wasm2c: support the custom-page-sizes proposal #2508  ON)
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

include(FetchContent)

FetchContent_Declare(
  rlbox
  GIT_REPOSITORY https://github.com/PLSysSec/rlbox.git)
FetchContent_GetProperties(rlbox)
if(NOT rlbox_POPULATED)
  FetchContent_Populate(rlbox)
endif()

FetchContent_Declare(catch2
                     GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                     GIT_TAG v2.13.9)
FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  FetchContent_Populate(catch2)
endif()

add_subdirectory("${catch2_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/contrib")

string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" CMAKE_HOST_SYSTEM_PROCESSOR_LOWER)

if (${CMAKE_HOST_SYSTEM_PROCESSOR_LOWER} MATCHES "x86_64" OR ${CMAKE_HOST_SYSTEM_PROCESSOR_LOWER} MATCHES "amd64")
  if (WIN32)
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-x86_64-windows.tar.gz")
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-x86_64-macos.tar.gz")
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-x86_64-linux.tar.gz")
  else()
    set (wasi-clang-url "")
  endif()
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR_LOWER} MATCHES "arm64")
  if (WIN32)
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-arm64-windows.tar.gz")
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-arm64-macos.tar.gz")
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (wasi-clang-url "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-27/wasi-sdk-27.0-arm64-linux.tar.gz")
  else()
    set (wasi-clang-url "")
  endif()
else()
  message(STATUS "Host platform found: ${CMAKE_HOST_SYSTEM_PROCESSOR_LOWER}")
  set (wasi-clang-url "")
endif()

if (WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  FetchContent_Declare(
    wasiclang
    DOWNLOAD_EXTRACT_TIMESTAMP false
    URL ${wasi-clang-url}
  )
  FetchContent_GetProperties(wasiclang)
  if(NOT wasiclang_POPULATED)
    FetchContent_Populate(wasiclang)
  endif()
  set(SHOULD_BUILD_WASICLANG OFF)
else()
  # We'll build wasi-clang this locally when we build this repo
  set(SHOULD_BUILD_WASICLANG ON)
  set(wasiclang_SOURCE_DIR "${CMAKE_SOURCE_DIR}/wasi-sdk-21-src/build/install/opt/wasi-sdk")
  message(STATUS "Prebuilt wasi-clang does not exist for this platform, so we have to build it from scratch. Your first build may take a long time!")
endif()

FetchContent_Declare(
  wasm2c_compiler
  GIT_REPOSITORY https://github.com/WebAssembly/wabt/
  GIT_TAG main)
FetchContent_GetProperties(wasm2c_compiler)
if(NOT wasm2c_compiler_POPULATED)
  FetchContent_Populate(wasm2c_compiler)
endif()

# set (wasm2c_compiler_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../wabt/")

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
  set(WASM2C_BUILD_DIR_SUFFIX "debug")
  set (WASM2C_BUILD_TYPE Debug)
else()
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(WASM2C_BUILD_DIR_SUFFIX "debug")
    set (WASM2C_BUILD_TYPE Debug)
  elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(WASM2C_BUILD_DIR_SUFFIX "relwithdebinfo")
    set (WASM2C_BUILD_TYPE RelWithDebInfo)
  elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set(WASM2C_BUILD_DIR_SUFFIX "minsizerel")
    set (WASM2C_BUILD_TYPE MinSizeRel)
  else()
    set(WASM2C_BUILD_DIR_SUFFIX "release")
    set (WASM2C_BUILD_TYPE Release)
  endif()
endif()

# wasm2c ###################

set(WASM2C_RUNTIME_SOURCE_DIR "${wasm2c_compiler_SOURCE_DIR}/wasm2c/")
set(WASM2C_RUNTIME_DIR "${wasm2c_compiler_SOURCE_DIR}/build_${WASM2C_BUILD_DIR_SUFFIX}/")

set(C_SOURCE_FILES "${CMAKE_SOURCE_DIR}/c_src/wasm2c_sandbox_wrapper.c")
set(GLUE_LIB_WASM_IMPORTED_DIR "${CMAKE_BINARY_DIR}/wasm_imported/")
set(GLUE_LIB_IMPORTED_WASM "${GLUE_LIB_WASM_IMPORTED_DIR}/glue_lib_wasm2c.wasm")
set(GLUE_LIB_IMPORTED_H "${GLUE_LIB_WASM_IMPORTED_DIR}/glue_lib_wasm2c.h")
set(GLUE_LIB_IMPORTED_C "${GLUE_LIB_WASM_IMPORTED_DIR}/glue_lib_wasm2c.c")
set(GLUE_LIB_WASM_EXPORTED_DIR "${CMAKE_BINARY_DIR}/wasm_exported/")
set(GLUE_LIB_EXPORTED_WASM "${GLUE_LIB_WASM_EXPORTED_DIR}/glue_lib_wasm2c.wasm")
set(GLUE_LIB_EXPORTED_H "${GLUE_LIB_WASM_EXPORTED_DIR}/glue_lib_wasm2c.h")
set(GLUE_LIB_EXPORTED_C "${GLUE_LIB_WASM_EXPORTED_DIR}/glue_lib_wasm2c.c")

if(MSVC)
  set(WASM2C_PATH ${WASM2C_RUNTIME_DIR}/${WASM2C_BUILD_TYPE}/wasm2c.exe)
else()
  set(WASM2C_PATH ${WASM2C_RUNTIME_DIR}/wasm2c)
endif()

add_custom_command(OUTPUT "${WASM2C_PATH}"
                   WORKING_DIRECTORY "${wasm2c_compiler_SOURCE_DIR}"
                   COMMAND ${CMAKE_COMMAND}
                           -DCMAKE_BUILD_TYPE=${WASM2C_BUILD_TYPE}
                           -DCMAKE_C_FLAGS="${RLBOX_SANITIZER_COMPILE_FLAGS}"
                           -DCMAKE_CXX_FLAGS="${RLBOX_SANITIZER_COMPILE_FLAGS}"
                           -DLINK_FLAGS="${RLBOX_SANITIZER_LINK_FLAGS}"
                           -S  .
                           -B ${WASM2C_RUNTIME_DIR}
                   COMMAND ${CMAKE_COMMAND}
                           --build ${WASM2C_RUNTIME_DIR}
                           --config ${WASM2C_BUILD_TYPE}
                           --parallel
                   COMMENT "Building wasm2c compiler and runtime")

set (WASICLANG_CHECKED "${CMAKE_BINARY_DIR}/wasiclang-checked.txt")

if (SHOULD_BUILD_WASICLANG)
  add_custom_command(OUTPUT "${WASICLANG_CHECKED}"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    COMMAND ./build-wasiclang.sh
                    COMMAND ${CMAKE_COMMAND} -E touch "${WASICLANG_CHECKED}"
                    COMMENT "Building wasi clang")
else()
  add_custom_command(OUTPUT "${WASICLANG_CHECKED}"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E touch "${WASICLANG_CHECKED}"
                    COMMENT "Using downloaded wasi-clang")
endif()

add_custom_command(OUTPUT "${GLUE_LIB_IMPORTED_H}" "${GLUE_LIB_IMPORTED_C}" "${GLUE_LIB_IMPORTED_WASM}"
                   DEPENDS ${C_SOURCE_FILES} ${WASM2C_PATH} ${WASICLANG_CHECKED}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLUE_LIB_WASM_IMPORTED_DIR}
                    COMMAND ${wasiclang_SOURCE_DIR}/bin/clang
                            --sysroot ${wasiclang_SOURCE_DIR}/share/wasi-sysroot/
                            -O3
                            -Wl,--export-all -Wl,--no-entry -Wl,--growable-table -Wl,--stack-first -Wl,-z,stack-size=1048576 -Wl,--import-memory -Wl,--import-table
                            -o ${GLUE_LIB_IMPORTED_WASM}
                            ${CMAKE_SOURCE_DIR}/c_src/wasm2c_sandbox_wrapper.c
                            ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib/libtest.c
                    COMMAND ${WASM2C_PATH}
                            -o ${GLUE_LIB_IMPORTED_C}
                            ${GLUE_LIB_IMPORTED_WASM}
                   COMMENT "Building wasm sandboxed library with imported memory and imported table")

add_custom_command(OUTPUT "${GLUE_LIB_EXPORTED_H}" "${GLUE_LIB_EXPORTED_C}" "${GLUE_LIB_EXPORTED_WASM}"
                   DEPENDS ${C_SOURCE_FILES} ${WASM2C_PATH} ${WASICLANG_CHECKED}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLUE_LIB_WASM_EXPORTED_DIR}
                    COMMAND ${wasiclang_SOURCE_DIR}/bin/clang
                            --sysroot ${wasiclang_SOURCE_DIR}/share/wasi-sysroot/
                            -O3
                            -Wl,--export-all -Wl,--no-entry -Wl,--growable-table -Wl,--stack-first -Wl,-z,stack-size=1048576 -Wl,--export-table
                            -o ${GLUE_LIB_EXPORTED_WASM}
                            ${CMAKE_SOURCE_DIR}/c_src/wasm2c_sandbox_wrapper.c
                            ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib/libtest.c
                    COMMAND ${WASM2C_PATH}
                            -o ${GLUE_LIB_EXPORTED_C}
                            ${GLUE_LIB_EXPORTED_WASM}
                   COMMENT "Building wasm sandboxed library with exported memory and exported table")

# Tests ###################

if(DEV)
  if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers ${RLBOX_SANITIZER_COMPILE_FLAGS})
    set_source_files_properties(${GLUE_LIB_IMPORTED_C} PROPERTIES COMPILE_OPTIONS "-Wno-cast-function-type;-Wno-unused-function;-Wno-unused-value;-Wno-unused-but-set-variable")
    set_source_files_properties(${GLUE_LIB_EXPORTED_C} PROPERTIES COMPILE_OPTIONS "-Wno-cast-function-type;-Wno-unused-function;-Wno-unused-value;-Wno-unused-but-set-variable")
    add_link_options(${RLBOX_SANITIZER_LINK_FLAGS})
  endif()
endif()

set(WASM2C_RUNTIME_CODE ${WASM2C_RUNTIME_SOURCE_DIR}/wasm-rt-impl.c
                        ${WASM2C_RUNTIME_SOURCE_DIR}/wasm-rt-mem-impl.c
                        ${CMAKE_SOURCE_DIR}/src/wasm2c_rt_minwasi.c
                        ${CMAKE_SOURCE_DIR}/src/wasm2c_rt_mem.c)

add_library(glue_lib_imported STATIC ${GLUE_LIB_IMPORTED_C} ${WASM2C_RUNTIME_CODE})
target_include_directories(glue_lib_imported PRIVATE ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                             PUBLIC ${wasm2c_compiler_SOURCE_DIR}/third_party/simde
                                             PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(glue_lib_imported PUBLIC WASM_RT_USE_MMAP=1
                                             PUBLIC WASM_RT_SKIP_SIGNAL_RECOVERY=1
                                             PUBLIC WASM_RT_NONCONFORMING_UNCHECKED_STACK_EXHAUSTION=1)

add_library(glue_lib_exported STATIC ${GLUE_LIB_EXPORTED_C} ${WASM2C_RUNTIME_CODE})
target_include_directories(glue_lib_exported PRIVATE ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                             PUBLIC ${wasm2c_compiler_SOURCE_DIR}/third_party/simde
                                             PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(glue_lib_exported PUBLIC WASM_RT_USE_MMAP=1
                                             PUBLIC WASM_RT_SKIP_SIGNAL_RECOVERY=1
                                             PUBLIC WASM_RT_NONCONFORMING_UNCHECKED_STACK_EXHAUSTION=1)

# Hack to avoid multiple concurrent calls to the custom command that builds wasm2c
add_dependencies(glue_lib_imported glue_lib_exported)

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
                                           PUBLIC ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                           PUBLIC ${GLUE_LIB_WASM_IMPORTED_DIR}
                                           )
target_link_libraries(test_rlbox_glue Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_imported
)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue rt)
endif()
catch_discover_tests(test_rlbox_glue)

####

add_executable(test_rlbox_glue_exported test/test_wasm2c_sandbox_glue_main.cpp
                                        test/test_wasm2c_sandbox_glue.cpp)
target_include_directories(test_rlbox_glue_exported PUBLIC ${CMAKE_SOURCE_DIR}/include
                                                    PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                                    PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                                    PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                                    PUBLIC ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                                    PUBLIC ${GLUE_LIB_WASM_EXPORTED_DIR}
                                                    )
target_link_libraries(test_rlbox_glue_exported Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_exported
)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_exported rt)
endif()
catch_discover_tests(test_rlbox_glue_exported)

####

add_executable(test_rlbox_glue_smallheap test/test_wasm2c_sandbox_glue_main.cpp
                               test/test_wasm2c_sandbox_glue_smallheap.cpp)
target_include_directories(test_rlbox_glue_smallheap PUBLIC ${CMAKE_SOURCE_DIR}/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/include
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue
                                           PUBLIC ${rlbox_SOURCE_DIR}/code/tests/rlbox_glue/lib
                                           PUBLIC ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                           PUBLIC ${GLUE_LIB_WASM_IMPORTED_DIR}
                                           )
target_link_libraries(test_rlbox_glue_smallheap Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_imported
)

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
                                           PUBLIC ${wasm2c_compiler_SOURCE_DIR}/wasm2c
                                           PUBLIC ${GLUE_LIB_WASM_IMPORTED_DIR}
                                           )
target_link_libraries(test_rlbox_glue_embed Catch2::Catch2
                                      ${CMAKE_THREAD_LIBS_INIT}
                                      ${CMAKE_DL_LIBS}
                                      glue_lib_imported
)

if(UNIX AND NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_link_libraries(test_rlbox_glue_embed rt)
endif()
catch_discover_tests(test_rlbox_glue_embed)

# Shortcuts ###################

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)
add_dependencies(check test_rlbox_glue)
add_dependencies(check test_rlbox_glue_exported)
add_dependencies(check test_rlbox_glue_smallheap)
add_dependencies(check test_rlbox_glue_embed)

