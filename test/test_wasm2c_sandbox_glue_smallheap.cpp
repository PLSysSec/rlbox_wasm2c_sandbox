#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol
#include "glue_lib_wasm2c.h"
#include "rlbox_wasm2c_sandbox.hpp"

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox smallheap"
// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox

// NOLINTNEXTLINE
#if defined(_WIN32)
#define CreateSandbox(sandbox) sandbox.create_sandbox(true /* abort on fail */, 8 * 1024 * 1024 /* max heap */)
#else
#define CreateSandbox(sandbox) sandbox.create_sandbox(true /* abort on fail */, 8 * 1024 * 1024 /* max heap */)
#endif
// NOLINTNEXTLINE
#include "test_sandbox_glue.inc.cpp"

// Global symbol lookup not yet supported
//#include "test_wasm2c_sandbox_wasmtests.cpp"
