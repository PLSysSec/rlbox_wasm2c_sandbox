#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>

// IWYU pragma: no_forward_declare mpl_::na
#include "catch2/catch.hpp"
#include "libtest.h"
#include "rlbox.hpp"

using rlbox::app_pointer;
using rlbox::rlbox_sandbox;
using rlbox::tainted;
using namespace std::chrono;

#ifndef CreateSandbox
#  error "Define CreateSandbox before including this file"
#endif

#ifndef TestName
#  error "Define TestName before including this file"
#endif

#ifndef TestType
#  error "Define TestType before including this file"
#endif

// NOLINTNEXTLINE
TEST_CASE("wasm sandbox glue tests " TestName, "[wasm_sandbox_glue_tests]")
{
  rlbox::rlbox_sandbox<TestType> sandbox;
  CreateSandbox(sandbox);

  SECTION("test double pointer access with bad value") // NOLINT
  {
    tainted<unsigned int*, TestType> ptr =
      sandbox.template malloc_in_sandbox<unsigned int>();
    // *ptr = 0xffffffff;
    *ptr = 2114112;

    tainted<unsigned int**, TestType> convPtr =
      rlbox::sandbox_reinterpret_cast<unsigned int**>(ptr);

    if constexpr (sizeof(uintptr_t) == sizeof(uint32_t)) {
      // RLBox's setup doesn't use guard pages for 32-bit sandboxes, so we need
      // to check that a dereference of an OOB actually traps
      REQUIRE_THROWS((tainted<unsigned int*, TestType>)*convPtr);

      REQUIRE_THROWS(** convPtr = 0);
    }

    sandbox.template free_in_sandbox(ptr);
  }

  SECTION("test tainted array indexing with bad value") // NOLINT
  {
    tainted<int, TestType> tnr = 34;

    int nr = tnr.copy_and_verify([](int nr) {
      if (!(nr >= 0)) {
        abort();
      }
      return nr;
    });

    tainted<char***, TestType> t_slst = sandbox.malloc_in_sandbox<char**>();
    *t_slst = sandbox.malloc_in_sandbox<char*>();
    **t_slst = sandbox.malloc_in_sandbox<char>();

    tainted<char**, TestType> t_slst_ref = *t_slst;

    // Pick a value "i" that when cast into a size_t and multiplied by 4 will go
    // out of bounds when used as in index, in 64-bit and 32-bit platforms
    // The default heap size by RLBox in 64-bit platforms is 4GB and 32-bit
    // platforms is 16MB
    //
    // We use int i = 0x55555555
    // - on 64-bit platforms static_cast<size_t>(0x55555555) * 4 is 0x155555554
    // which is outside 4GB heap used
    // - on 32-bit platforms static_cast<size_t>(0x55555555) * 4 is 0x55555554
    // which is outside 16MB heap used
    int i = 0x55555555; //(INT32_MAX / 1.5);

    REQUIRE_THROWS(t_slst_ref[i]);
  }

  sandbox.destroy_sandbox();
}