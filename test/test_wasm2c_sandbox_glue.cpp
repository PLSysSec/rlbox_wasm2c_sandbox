#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol
#define RLBOX_WASM2C_MODULE_NAME glue__lib__wasm2c
#include "glue_lib_wasm2c.h" // IWYU pragma: keep
#include "rlbox_wasm2c_sandbox.hpp" // IWYU pragma: keep

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox static"

// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox

// NOLINTNEXTLINE
#define CreateSandbox(sandbox) sandbox.create_sandbox()

// NOLINTNEXTLINE
#include "test_sandbox_glue.inc.cpp"

