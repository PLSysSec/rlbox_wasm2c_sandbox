#include <cstdint>

// IWYU pragma: no_forward_declare mpl_::na
#include "catch2/catch.hpp"
#include "libtest.h"
#include "rlbox.hpp"

#ifndef CreateSandbox
#  error "Define CreateSandbox before including this file"
#endif

#ifndef CreateSandboxFallible
#  error "Define CreateSandboxFallible before including this file"
#endif

#ifndef TestName
#  error "Define TestName before including this file"
#endif

#ifndef TestType
#  error "Define TestType before including this file"
#endif


TEST_CASE("wasm sandbox tests " TestName, "[wasm_sandbox_tests]")
{
    rlbox::rlbox_sandbox<TestType> sandbox;
    CreateSandbox(sandbox);

    SECTION("global symbol lookup")
    {
      uint32_t* errno_ptr = (uint32_t*) sandbox.lookup_symbol("errno");
      REQUIRE(errno_ptr != nullptr);
    }

    sandbox.destroy_sandbox();
}

TEST_CASE("wasm sandbox fallible create " TestName, "[wasm_sandbox_tests]")
{
  rlbox::rlbox_sandbox<TestType> sandbox;
  bool ret = CreateSandboxFallible(sandbox);
  REQUIRE(ret == false);
}
