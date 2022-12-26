// Based on
// https://web.archive.org/web/20191012035921/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system#BSD
// Check for windows (non cygwin) environment
#if defined(_WIN32)

#include "wasm-rt-os.h"
#include "wasm-rt.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#ifdef VERBOSE_LOGGING
#define VERBOSE_LOG(...) \
  { printf(__VA_ARGS__); }
#else
#define VERBOSE_LOG(...)
#endif

#define DONT_USE_VIRTUAL_ALLOC2

size_t os_getpagesize() {
  SYSTEM_INFO S;
  GetNativeSystemInfo(&S);
  return S.dwPageSize;
}

static void* win_mmap(void* hint,
                      size_t size,
                      int prot,
                      int flags,
                      DWORD alloc_flag) {
  DWORD flProtect = PAGE_NOACCESS;
  size_t request_size, page_size;
  void* addr;

  page_size = os_getpagesize();
  request_size = (size + page_size - 1) & ~(page_size - 1);

  if (request_size < size)
    /* integer overflow */
    return NULL;

  if (request_size == 0)
    request_size = page_size;

  if (prot & MMAP_PROT_EXEC) {
    if (prot & MMAP_PROT_WRITE)
      flProtect = PAGE_EXECUTE_READWRITE;
    else
      flProtect = PAGE_EXECUTE_READ;
  } else if (prot & MMAP_PROT_WRITE)
    flProtect = PAGE_READWRITE;
  else if (prot & MMAP_PROT_READ)
    flProtect = PAGE_READONLY;

  addr = VirtualAlloc((LPVOID)hint, request_size, alloc_flag, flProtect);
  return addr;
}

void* os_mmap(void* hint, size_t size, int prot, int flags) {
  DWORD alloc_flag = MEM_RESERVE | MEM_COMMIT;
  return win_mmap(hint, size, prot, flags, alloc_flag);
}

#ifndef DONT_USE_VIRTUAL_ALLOC2
static void* win_mmap_aligned(void* hint,
                              size_t size,
                              int prot,
                              int flags,
                              size_t pow2alignment) {
  DWORD alloc_flag = MEM_RESERVE | MEM_COMMIT;
  DWORD flProtect = PAGE_NOACCESS;
  size_t request_size, page_size;
  void* addr;

  page_size = os_getpagesize();
  request_size = (size + page_size - 1) & ~(page_size - 1);

  if (request_size < size)
    /* integer overflow */
    return NULL;

  if (request_size == 0)
    request_size = page_size;

  if (prot & MMAP_PROT_EXEC) {
    if (prot & MMAP_PROT_WRITE)
      flProtect = PAGE_EXECUTE_READWRITE;
    else
      flProtect = PAGE_EXECUTE_READ;
  } else if (prot & MMAP_PROT_WRITE)
    flProtect = PAGE_READWRITE;
  else if (prot & MMAP_PROT_READ)
    flProtect = PAGE_READONLY;

  MEM_ADDRESS_REQUIREMENTS addressReqs = {0};
  MEM_EXTENDED_PARAMETER param = {0};

  addressReqs.Alignment = pow2alignment;
  addressReqs.HighestEndingAddress = 0;
  addressReqs.LowestStartingAddress = 0;

  param.Type = MemExtendedParameterAddressRequirements;
  param.Pointer = &addressReqs;

  addr = VirtualAlloc2(0, (LPVOID)addr, request_size, alloc_flag, flProtect,
                       &param, 1);
  return addr;
}
#endif

void os_munmap(void* addr, size_t size) {
  DWORD alloc_flag = MEM_RELEASE;
  if (addr) {
    if (VirtualFree(addr, 0, alloc_flag) == 0) {
      size_t page_size = os_getpagesize();
      size_t request_size = (size + page_size - 1) & ~(page_size - 1);
      int64_t curr_err = errno;
      printf("os_munmap error addr:%p, size:0x%zx, errno:%" PRId64 "\n", addr,
             request_size, curr_err);
    }
  }
}

int os_mprotect(void* addr, size_t size, int prot) {
  DWORD flProtect = PAGE_NOACCESS;

  if (!addr)
    return 0;

  if (prot & MMAP_PROT_EXEC) {
    if (prot & MMAP_PROT_WRITE)
      flProtect = PAGE_EXECUTE_READWRITE;
    else
      flProtect = PAGE_EXECUTE_READ;
  } else if (prot & MMAP_PROT_WRITE)
    flProtect = PAGE_READWRITE;
  else if (prot & MMAP_PROT_READ)
    flProtect = PAGE_READONLY;

  DWORD old;
  BOOL succeeded = VirtualProtect((LPVOID)addr, size, flProtect, &old);
  return succeeded ? 0 : -1;
}

#ifndef DONT_USE_VIRTUAL_ALLOC2
static int IsPowerOfTwoOrZero(size_t x) {
  return (x & (x - 1)) == 0;
}
#endif

void* os_mmap_aligned(void* addr,
                      size_t requested_length,
                      int prot,
                      int flags,
                      size_t alignment,
                      size_t alignment_offset) {
#ifndef DONT_USE_VIRTUAL_ALLOC2
  if (IsPowerOfTwoOrZero(alignment) && alignment_offset == 0) {
    return win_mmap_aligned(addr, requested_length, prot, flags, alignment);
  } else
#endif
  {
    size_t padded_length = requested_length + alignment + alignment_offset;
    uintptr_t unaligned =
        (uintptr_t)win_mmap(addr, padded_length, prot, flags, MEM_RESERVE);

    VERBOSE_LOG(
        "os_mmap_aligned: alignment:%llu, alignment_offset:%llu, "
        "requested_length:%llu, padded_length: %llu, initial mapping: %p\n",
        (unsigned long long)alignment, (unsigned long long)alignment_offset,
        (unsigned long long)requested_length, (unsigned long long)padded_length,
        (void*)unaligned);

    if (!unaligned) {
      return (void*)unaligned;
    }

    // Round up the next address that has addr % alignment = 0
    const size_t alignment_corrected = alignment == 0 ? 1 : alignment;
    uintptr_t aligned_nonoffset =
        (unaligned + (alignment_corrected - 1)) & ~(alignment_corrected - 1);

    // Currently offset 0 is aligned according to alignment
    // Alignment needs to be enforced at the given offset
    uintptr_t aligned = 0;
    if ((aligned_nonoffset - alignment_offset) >= unaligned) {
      aligned = aligned_nonoffset - alignment_offset;
    } else {
      aligned = aligned_nonoffset - alignment_offset + alignment;
    }

    if (aligned == unaligned && padded_length == requested_length) {
      return (void*)aligned;
    }

    // Sanity check
    if (aligned < unaligned ||
        (aligned + (requested_length - 1)) >
            (unaligned + (padded_length - 1)) ||
        (aligned + alignment_offset) % alignment_corrected != 0) {
      VERBOSE_LOG("os_mmap_aligned: sanity check fail. aligned: %p\n",
                  (void*)aligned);
      os_munmap((void*)unaligned, padded_length);
      return NULL;
    }

    // windows does not support partial unmapping, so unmap and remap
    os_munmap((void*)unaligned, padded_length);
    aligned = (uintptr_t)win_mmap((void*)aligned, requested_length, prot, flags,
                                  MEM_RESERVE);
    VERBOSE_LOG("os_mmap_aligned: final mapping: %p\n", (void*)aligned);
    return (void*)aligned;
  }
}

int os_mmap_commit(void* curr_heap_end_pointer,
                   size_t expanded_size,
                   int prot) {
  uintptr_t addr = (uintptr_t)win_mmap(curr_heap_end_pointer, expanded_size,
                                       prot, MMAP_MAP_NONE, MEM_COMMIT);
  int ret = addr ? 0 : -1;
  return ret;
}

typedef struct {
  LARGE_INTEGER counts_per_sec;
} wasi_win_clock_info_t;

static wasi_win_clock_info_t g_wasi_win_clock_info;
static int g_os_data_initialized = 0;

void os_init() {
  // From here:
  // https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows/38212960#38212960
  if (QueryPerformanceFrequency(&g_wasi_win_clock_info.counts_per_sec) == 0) {
    wasm_rt_trap(WASM_RT_TRAP_WASI);
  }
  g_os_data_initialized = 1;
}

void os_clock_init(void** clock_data_pointer) {
  if (!g_os_data_initialized) {
    os_init();
  }

  wasi_win_clock_info_t* alloc =
      (wasi_win_clock_info_t*)malloc(sizeof(wasi_win_clock_info_t));
  if (!alloc) {
    wasm_rt_trap(WASM_RT_TRAP_WASI);
  }
  memcpy(alloc, &g_wasi_win_clock_info, sizeof(wasi_win_clock_info_t));
  *clock_data_pointer = alloc;
}

void os_clock_cleanup(void** clock_data_pointer) {
  if (*clock_data_pointer == 0) {
    free(*clock_data_pointer);
    *clock_data_pointer = 0;
  }
}

int os_clock_gettime(void* clock_data,
                     int clock_id,
                     struct timespec* out_struct) {
  wasi_win_clock_info_t* alloc = (wasi_win_clock_info_t*)clock_data;

  LARGE_INTEGER count;
  (void)clock_id;

  if (alloc->counts_per_sec.QuadPart <= 0 ||
      QueryPerformanceCounter(&count) == 0) {
    return -1;
  }

#define BILLION 1000000000LL
  out_struct->tv_sec = count.QuadPart / alloc->counts_per_sec.QuadPart;
  out_struct->tv_nsec =
      ((count.QuadPart % alloc->counts_per_sec.QuadPart) * BILLION) /
      alloc->counts_per_sec.QuadPart;
#undef BILLION

  return 0;
}

int os_clock_getres(void* clock_data,
                    int clock_id,
                    struct timespec* out_struct) {
  (void)clock_id;
  out_struct->tv_sec = 0;
  out_struct->tv_nsec = 1000;
  return 0;
}

void os_print_last_error(const char* msg) {
  DWORD errorMessageID = GetLastError();
  if (errorMessageID != 0) {
    LPSTR messageBuffer = 0;
    // The api creates the buffer that holds the message
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    (void)size;
    // Copy the error message into a std::string.
    printf("%s. %s\n", msg, messageBuffer);
    LocalFree(messageBuffer);
  } else {
    printf("%s. No error code.\n", msg);
  }
}

#undef VERBOSE_LOG
#undef DONT_USE_VIRTUAL_ALLOC2

#elif !defined(_WIN32) && (defined(__unix__) || defined(__unix) || \
                         (defined(__APPLE__) && defined(__MACH__)))

#include "wasm-rt-os.h"
#include "wasm-rt.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && defined(__MACH__)
// Macs priors to OSX 10.12 don't have the clock functions. So we will use mac
// specific options
#include <mach/mach_time.h>
#include <sys/time.h>
#endif
#include <sys/mman.h>
#include <unistd.h>

#ifdef VERBOSE_LOGGING
#define VERBOSE_LOG(...) \
  { printf(__VA_ARGS__); }
#else
#define VERBOSE_LOG(...)
#endif

size_t os_getpagesize() {
  return getpagesize();
}

void* os_mmap(void* hint, size_t size, int prot, int flags) {
  int map_prot = PROT_NONE;
  int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
  uint64_t request_size, page_size;
  uint8_t* addr;

  page_size = (uint64_t)os_getpagesize();
  request_size = (size + page_size - 1) & ~(page_size - 1);

  if ((size_t)request_size < size)
    /* integer overflow */
    return NULL;

  if (request_size > 16 * (uint64_t)UINT32_MAX)
    /* At most 16 G is allowed */
    return NULL;

  if (prot & MMAP_PROT_READ)
    map_prot |= PROT_READ;

  if (prot & MMAP_PROT_WRITE)
    map_prot |= PROT_WRITE;

  if (prot & MMAP_PROT_EXEC)
    map_prot |= PROT_EXEC;

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
#ifndef __APPLE__
  if (flags & MMAP_MAP_32BIT)
    map_flags |= MAP_32BIT;
#endif
#endif

  if (flags & MMAP_MAP_FIXED)
    map_flags |= MAP_FIXED;

  addr = mmap(hint, request_size, map_prot, map_flags, -1, 0);

  if (addr == MAP_FAILED)
    return NULL;

  return addr;
}

void os_munmap(void* addr, size_t size) {
  uint64_t page_size = (uint64_t)os_getpagesize();
  uint64_t request_size = (size + page_size - 1) & ~(page_size - 1);

  if (addr) {
    if (munmap(addr, request_size)) {
      printf("os_munmap error addr:%p, size:0x%" PRIx64 ", errno:%d\n", addr,
             request_size, errno);
    }
  }
}

int os_mprotect(void* addr, size_t size, int prot) {
  int map_prot = PROT_NONE;
  uint64_t page_size = (uint64_t)os_getpagesize();
  uint64_t request_size = (size + page_size - 1) & ~(page_size - 1);

  if (!addr)
    return 0;

  if (prot & MMAP_PROT_READ)
    map_prot |= PROT_READ;

  if (prot & MMAP_PROT_WRITE)
    map_prot |= PROT_WRITE;

  if (prot & MMAP_PROT_EXEC)
    map_prot |= PROT_EXEC;

  return mprotect(addr, request_size, map_prot);
}

void* os_mmap_aligned(void* addr,
                      size_t requested_length,
                      int prot,
                      int flags,
                      size_t alignment,
                      size_t alignment_offset) {
  size_t padded_length = requested_length + alignment + alignment_offset;
  uintptr_t unaligned = (uintptr_t)os_mmap(addr, padded_length, prot, flags);

  VERBOSE_LOG(
      "os_mmap_aligned: alignment:%llu, alignment_offset:%llu, "
      "requested_length:%llu, padded_length: %llu, initial mapping: %p\n",
      (unsigned long long)alignment, (unsigned long long)alignment_offset,
      (unsigned long long)requested_length, (unsigned long long)padded_length,
      (void*)unaligned);

  if (!unaligned) {
    return (void*)unaligned;
  }

  // Round up the next address that has addr % alignment = 0
  const size_t alignment_corrected = alignment == 0 ? 1 : alignment;
  uintptr_t aligned_nonoffset =
      (unaligned + (alignment_corrected - 1)) & ~(alignment_corrected - 1);

  // Currently offset 0 is aligned according to alignment
  // Alignment needs to be enforced at the given offset
  uintptr_t aligned = 0;
  if ((aligned_nonoffset - alignment_offset) >= unaligned) {
    aligned = aligned_nonoffset - alignment_offset;
  } else {
    aligned = aligned_nonoffset - alignment_offset + alignment;
  }

  // Sanity check
  if (aligned < unaligned ||
      (aligned + (requested_length - 1)) > (unaligned + (padded_length - 1)) ||
      (aligned + alignment_offset) % alignment_corrected != 0) {
    VERBOSE_LOG("os_mmap_aligned: sanity check fail. aligned: %p\n",
                (void*)aligned);
    os_munmap((void*)unaligned, padded_length);
    return NULL;
  }

  {
    size_t unused_front = aligned - unaligned;
    if (unused_front != 0) {
      os_munmap((void*)unaligned, unused_front);
    }
  }

  {
    size_t unused_back =
        (unaligned + (padded_length - 1)) - (aligned + (requested_length - 1));
    if (unused_back != 0) {
      os_munmap((void*)(aligned + requested_length), unused_back);
    }
  }

  VERBOSE_LOG("os_mmap_aligned: final mapping: %p\n", (void*)aligned);
  return (void*)aligned;
}

int os_mmap_commit(void* curr_heap_end_pointer,
                   size_t expanded_size,
                   int prot) {
  return os_mprotect(curr_heap_end_pointer, expanded_size, prot);
}

#if defined(__APPLE__) && defined(__MACH__)
typedef struct {
  mach_timebase_info_data_t timebase; /* numer = 0, denom = 0 */
  struct timespec inittime; /* nanoseconds since 1-Jan-1970 to init() */
  uint64_t initclock;       /* ticks since boot to init() */
} wasi_mac_clock_info_t;

static wasi_mac_clock_info_t g_wasi_mac_clock_info;
static int g_os_data_initialized = 0;
#endif

void os_init() {
#if defined(__APPLE__) && defined(__MACH__)
  // From here:
  // https://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x/21352348#21352348
  if (mach_timebase_info(&g_wasi_mac_clock_info.timebase) != 0) {
    wasm_rt_trap(WASM_RT_TRAP_WASI);
  }

  // microseconds since 1 Jan 1970
  struct timeval micro;
  if (gettimeofday(&micro, NULL) != 0) {
    wasm_rt_trap(WASM_RT_TRAP_WASI);
  }

  g_wasi_mac_clock_info.initclock = mach_absolute_time();

  g_wasi_mac_clock_info.inittime.tv_sec = micro.tv_sec;
  g_wasi_mac_clock_info.inittime.tv_nsec = micro.tv_usec * 1000;

  g_os_data_initialized = 1;
#endif
}

void os_clock_init(void** clock_data_pointer) {
#if defined(__APPLE__) && defined(__MACH__)
  if (!g_os_data_initialized) {
    os_init();
  }

  wasi_mac_clock_info_t* alloc =
      (wasi_mac_clock_info_t*)malloc(sizeof(wasi_mac_clock_info_t));
  if (!alloc) {
    wasm_rt_trap(WASM_RT_TRAP_WASI);
  }
  memcpy(alloc, &g_wasi_mac_clock_info, sizeof(wasi_mac_clock_info_t));
  *clock_data_pointer = alloc;
#endif
}

void os_clock_cleanup(void** clock_data_pointer) {
#if defined(__APPLE__) && defined(__MACH__)
  if (*clock_data_pointer == 0) {
    free(*clock_data_pointer);
    *clock_data_pointer = 0;
  }
#endif
}

int os_clock_gettime(void* clock_data,
                     int clock_id,
                     struct timespec* out_struct) {
  int ret = 0;
#if defined(__APPLE__) && defined(__MACH__)
  wasi_mac_clock_info_t* alloc = (wasi_mac_clock_info_t*)clock_data;

  // From here:
  // https://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x/21352348#21352348

  (void)clock_id;
  // ticks since init
  uint64_t clock = mach_absolute_time() - alloc->initclock;
  // nanoseconds since init
  uint64_t nano = clock * (uint64_t)(alloc->timebase.numer) /
                  (uint64_t)(alloc->timebase.denom);
  *out_struct = alloc->inittime;

#define BILLION 1000000000L
  out_struct->tv_sec += nano / BILLION;
  out_struct->tv_nsec += nano % BILLION;
  // normalize
  out_struct->tv_sec += out_struct->tv_nsec / BILLION;
  out_struct->tv_nsec = out_struct->tv_nsec % BILLION;
#undef BILLION
#else
  ret = clock_gettime(clock_id, out_struct);
#endif
  return ret;
}

int os_clock_getres(void* clock_data,
                    int clock_id,
                    struct timespec* out_struct) {
  int ret = 0;
#if defined(__APPLE__) && defined(__MACH__)
  (void)clock_id;
  out_struct->tv_sec = 0;
  out_struct->tv_nsec = 1;
#else
  ret = clock_getres(clock_id, out_struct);
#endif
  return ret;
}

void os_print_last_error(const char* msg) {
  perror(msg);
}

#undef VERBOSE_LOG

#else
#error "Unknown OS"
#endif