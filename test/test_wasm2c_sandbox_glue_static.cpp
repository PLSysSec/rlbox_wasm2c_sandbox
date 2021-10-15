#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol
#include "glue_lib_wasm2c.h"
#include "rlbox_wasm2c_sandbox.hpp"

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox static"
// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox

#ifndef GLUE_LIB_WASM2C_PATH
#  error "Missing definition for GLUE_LIB_WASM2C_PATH"
#endif

// NOLINTNEXTLINE
#if defined(_WIN32)
#define CreateSandbox(sandbox) sandbox.create_sandbox()
#else
#define CreateSandbox(sandbox) sandbox.create_sandbox()
#endif
// NOLINTNEXTLINE
#include "test_sandbox_glue.inc.cpp"

// Global symbol lookup not yet supported
//#include "test_wasm2c_sandbox_wasmtests.cpp"
