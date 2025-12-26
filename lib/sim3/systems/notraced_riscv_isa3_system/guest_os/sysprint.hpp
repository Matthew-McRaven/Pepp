#pragma once
#include <string>
#define SYSCALL_VERBOSE 1
#ifdef SYSCALL_VERBOSE
#define SYSPRINT(fmt, ...)                                                                                             \
  {                                                                                                                    \
    char syspbuf[1024];                                                                                                \
    machine.print(syspbuf, snprintf(syspbuf, sizeof(syspbuf), fmt, ##__VA_ARGS__));                                    \
  }
static constexpr bool verbose_syscalls = true;
#else
#define SYSPRINT(fmt, ...) /* fmt */
static constexpr bool verbose_syscalls = false;
#endif
