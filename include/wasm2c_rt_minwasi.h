#ifndef WASM_RT_MINWASI_H_
#define WASM_RT_MINWASI_H_

/* A minimum wasi implementation supporting only stdin, stdout, stderr, argv
 * (upto 100 args) and clock functions. */

#include <stdint.h>

#include "wasm-rt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Z_wasi_snapshot_preview1_instance_t {
  wasm_rt_memory_t* instance_memory;

  uint32_t main_argc;
  const char** main_argv;

  uint32_t env_count;
  const char** env;

  void* clock_data;
} Z_wasi_snapshot_preview1_instance_t;

void minwasi_init();
void minwasi_init_instance(Z_wasi_snapshot_preview1_instance_t* wasi_data);
void minwasi_cleanup_instance(Z_wasi_snapshot_preview1_instance_t* wasi_data);

#ifdef __cplusplus
}
#endif

#endif
