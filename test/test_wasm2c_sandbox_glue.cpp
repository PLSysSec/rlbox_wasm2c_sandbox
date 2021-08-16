#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#include "rlbox_wasm2c_sandbox.hpp"

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox"
// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox

#ifndef GLUE_LIB_WASM2C_PATH
#  error "Missing definition for GLUE_LIB_WASM2C_PATH"
#endif

// NOLINTNEXTLINE
#define CreateSandbox(sandbox) sandbox.create_sandbox(GLUE_LIB_WASM2C_PATH)
// NOLINTNEXTLINE
#include "test_sandbox_glue.inc.cpp"
#include "test_wasm2c_sandbox_wasmtests.cpp"
