#ifndef WASM_RT_OS_H_
#define WASM_RT_OS_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "wasm-rt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Z_env_instance_t {
  wasm_rt_memory_t* sandbox_memory_info;
};

wasm_rt_memory_t* Z_envZ_memory(struct Z_env_instance_t* instance);

wasm_rt_memory_t create_wasm2c_memory(uint32_t initial_pages, uint64_t override_max_wasm_pages);
void destroy_wasm2c_memory(wasm_rt_memory_t* memory);

#ifdef __cplusplus
}
#endif

#endif
