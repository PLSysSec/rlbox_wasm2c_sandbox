#define RLBOX_USE_EXCEPTIONS
#define RLBOX_ENABLE_DEBUG_ASSERTIONS
#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol
#define RLBOX_WASM2C_MODULE_NAME glue__lib__wasm2c
#define RLBOX_EMBEDDER_PROVIDES_TLS_STATIC_VARIABLES
#include "glue_lib_wasm2c.h"
#include "rlbox_wasm2c_sandbox.hpp"
RLBOX_WASM2C_SANDBOX_STATIC_VARIABLES();

// NOLINTNEXTLINE
#define TestName "rlbox_wasm2c_sandbox embedder"

DEFINE_RLBOX_WASM2C_MODULE_TYPE(glue__lib__wasm2c);

// NOLINTNEXTLINE
#define TestType rlbox::rlbox_wasm2c_sandbox<rlbox_wasm2c_module_type_glue__lib__wasm2c>

// NOLINTNEXTLINE
#define CreateSandbox(sandbox) sandbox.create_sandbox()

// NOLINTNEXTLINE
#include "test_sandbox_glue.inc.cpp"
