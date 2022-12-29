#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol
#include "glue_lib_wasm2c.h"
#include "rlbox_wasm2c_sandbox.hpp"

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox static"

DEFINE_RLBOX_WASM2C_MODULE_TYPE(glue_lib_wasm2c);

// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox<rlbox_wasm2c_module_type_glue_lib_wasm2c>

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
