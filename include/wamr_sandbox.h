#ifndef WAMR_SANDBOX_H
#define WAMR_SANDBOX_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct WamrSandboxInstance;

typedef struct WamrSandboxInstance WamrSandboxInstance;

enum WamrValueType {
  WamrValueType_I32,
  WamrValueType_I64,
  WamrValueType_F32,
  WamrValueType_F64,
  WamrValueType_Void
};

typedef struct {
  enum WamrValueType val_type;
  union {
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
  };
} WamrValue;

typedef struct {
  enum WamrValueType ret;
  uint32_t parameter_cnt;
  WamrValueType *parameters;
} WamrFunctionSignature;

WamrSandboxInstance *wamr_load_module(const char *wamr_module_path);
void wamr_drop_module(WamrSandboxInstance *inst);

void* wamr_lookup_function(WamrSandboxInstance *inst,
                            const char *fn_name);
void wamr_set_curr_instance(WamrSandboxInstance *inst);
void wamr_clear_curr_instance(WamrSandboxInstance *inst);

uintptr_t wamr_get_func_call_env_param(WamrSandboxInstance* inst);

uint32_t wamr_register_callback(WamrSandboxInstance* inst, WamrFunctionSignature csig, const void* func_ptr);
uint32_t wamr_register_internal_callback(WamrSandboxInstance* inst, WamrFunctionSignature csig, const void* func_ptr);
void wamr_unregister_callback(WamrSandboxInstance* inst, uint32_t slot);

void *wamr_get_heap_base(WamrSandboxInstance *inst);
size_t wamr_get_heap_size(WamrSandboxInstance *inst);
uint32_t wamr_get_export_function_id(void *inst, void *unsandboxed_ptr);

#ifdef __cplusplus
}
#endif

#endif