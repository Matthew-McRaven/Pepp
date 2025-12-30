#include <stddef.h>
#include <stdint.h>

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#ifndef NATIVE_SYSCALLS_BASE
#define NATIVE_SYSCALLS_BASE   470
#endif
#ifndef THREAD_SYSCALLS_BASE
#define THREAD_SYSCALLS_BASE   490
#endif

#define SYSCALL_MALLOC    (NATIVE_SYSCALLS_BASE+0)
#define SYSCALL_CALLOC    (NATIVE_SYSCALLS_BASE+1)
#define SYSCALL_REALLOC   (NATIVE_SYSCALLS_BASE+2)
#define SYSCALL_FREE      (NATIVE_SYSCALLS_BASE+3)
#define SYSCALL_MEMINFO   (NATIVE_SYSCALLS_BASE+4)

#define SYSCALL_MEMCPY    (NATIVE_SYSCALLS_BASE+5)
#define SYSCALL_MEMSET    (NATIVE_SYSCALLS_BASE+6)
#define SYSCALL_MEMMOVE   (NATIVE_SYSCALLS_BASE+7)
#define SYSCALL_MEMCMP    (NATIVE_SYSCALLS_BASE+8)

#define SYSCALL_STRLEN    (NATIVE_SYSCALLS_BASE+10)
#define SYSCALL_STRCMP    (NATIVE_SYSCALLS_BASE+11)

#define SYSCALL_BACKTRACE (NATIVE_SYSCALLS_BASE+19)

#define ASM_MAX_BUFSZ  16384U

#ifdef __cplusplus
extern "C" {
#endif

static inline void fail() {
  register long syscall_id __asm__("a7") = SYSCALL_EXIT;

  __asm__ volatile("ecall" ::"r"(syscall_id) :);
}

static inline long write(int fd, const void *data, unsigned long size) {
  register int a0 __asm__("a0") = fd;
  register char *a1 __asm__("a1") = (char *)data;
  register unsigned long a2 __asm__("a2") = size;
  register long syscall_id __asm__("a7") = SYSCALL_WRITE;
  register long a0_out __asm__("a0");

  __asm__ volatile("ecall"
                   : "+r"(a0) // a0 updated by kernel (return)
                   : "r"(a1), "r"(a2), "r"(syscall_id)
                   : "memory");
  return a0_out;
}

static inline
void* memset(void* vdest, const int ch, size_t size)
{
	register char*   a0 __asm__("a0") = (char*)vdest;
	register int     a1 __asm__("a1") = ch;
	register size_t  a2 __asm__("a2") = size;
	register long syscall_id __asm__("a7") = SYSCALL_MEMSET;

	__asm__ volatile ("ecall"
	:	"=m"(*(char(*)[size]) a0)
	:	"r"(a0), "r"(a1), "r"(a2), "r"(syscall_id));
	return vdest;
}

void *memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++) ((char *)dest)[i] = ((char *)src)[i];
  return dest;
}

static inline
void* memmove(void* vdest, const void* vsrc, size_t size)
{
	// An assumption is being made here that since vsrc might be
	// inside vdest, we cannot assume that vsrc is const anymore.
	register char*  a0 __asm__("a0") = (char*)vdest;
	register char*  a1 __asm__("a1") = (char*)vsrc;
	register size_t a2 __asm__("a2") = size;
	register long syscall_id __asm__("a7") = SYSCALL_MEMMOVE;

	__asm__ volatile ("ecall"
		: "=m"(*(char(*)[size]) a0), "=m"(*(char(*)[size]) a1)
		: "r"(a0), "r"(a1), "r"(a2), "r"(syscall_id));
	return vdest;
}

static inline
int memcmp(const void* m1, const void* m2, size_t size)
{
	register const char* a0  __asm__("a0") = (const char*)m1;
	register const char* a1  __asm__("a1") = (const char*)m2;
	register size_t      a2  __asm__("a2") = size;
	register long syscall_id __asm__("a7") = SYSCALL_MEMCMP;
	register int         a0_out __asm__("a0");

	__asm__ volatile ("ecall" : "=r"(a0_out) :
		"r"(a0), "m"(*(const char(*)[size]) a0),
		"r"(a1), "m"(*(const char(*)[size]) a1),
		"r"(a2), "r"(syscall_id));
	return a0_out;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define GENERATE_SYSCALL_WRAPPER(name, number) \
	__asm__(".global " #name "\n" #name ":\n  li a7, " STRINGIFY(number) "\n  ecall\n  ret\n"); \

GENERATE_SYSCALL_WRAPPER(sys_malloc,  SYSCALL_MALLOC);
GENERATE_SYSCALL_WRAPPER(sys_calloc,  SYSCALL_CALLOC);
GENERATE_SYSCALL_WRAPPER(sys_realloc, SYSCALL_REALLOC);
GENERATE_SYSCALL_WRAPPER(sys_free,    SYSCALL_FREE);

extern void *sys_malloc(size_t);
extern void *sys_calloc(size_t, size_t);
extern void *sys_realloc(void *, size_t);
extern void *sys_free(void *);

inline void *malloc(size_t size) { return sys_malloc(size); }
inline void *calloc(size_t count, size_t size) { return sys_calloc(count, size); }
inline void *realloc(void *ptr, size_t newsize) { return sys_realloc(ptr, newsize); }
inline void free(void *ptr) { (void)sys_free(ptr); }
inline void *reallocf(void *ptr, size_t newsize) {
  void *newptr = realloc(ptr, newsize);
  if (newptr == NULL) free(ptr);
  return newptr;
}

static inline void *memalign(size_t align, size_t bytes) {
  // XXX: TODO: Make an accelerated memalign system call
  void *freelist[1024]; // Enough for 4K alignment
  size_t freecounter = 0;
  void *ptr = NULL;

  while (1) {
		ptr = sys_malloc(bytes);
		if (ptr == NULL) break;
		int aligned = ((uintptr_t)ptr & (align-1)) == 0;
		if (aligned) break;
		sys_free(ptr);
		// Allocate 8 bytes to advance the next pointer
		freelist[freecounter++] = sys_malloc(8);
	}

	for (size_t i = 0; i < freecounter; i++) sys_free(freelist[i]);
	return ptr;
}

static inline int posix_memalign(void **memptr, size_t alignment, size_t size) {
  void *ptr = memalign(alignment, size);
  *memptr = ptr;
  return 0;
}
static inline void *aligned_alloc(size_t alignment, size_t size) { return memalign(alignment, size); }

static inline wchar_t *wmemcpy(wchar_t *wto, const wchar_t *wfrom, size_t size) {
  return (wchar_t *)memcpy(wto, wfrom, size * sizeof(wchar_t));
}

static inline void *memchr(const void *s, int c, size_t n) {
  if (n != 0) {
    const unsigned char *p = (const unsigned char *)s;

    do {
      if (*p++ == c) return ((void *)(p - 1));
    } while (--n != 0);
  }
  return nullptr;
}

static inline char *strcpy(char *dst, const char *src) {
  while ((*dst++ = *src++));
  *dst = 0;
  return dst;
}
static inline size_t strlen(const char *str) {
  const char *iter;
  for (iter = str; *iter; ++iter);
  return iter - str;
}
static inline int strcmp(const char *str1, const char *str2) {
  while (*str1 != 0 && *str2 != 0 && *str1 == *str2) str1++, str2++;
  return *str1 - *str2;
}
static inline int strncmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && (*s1 == *s2)) ++s1, ++s2, --n;

  if (n == 0) return 0;
  else return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

static inline char *strcat(char *dest, const char *src) {
  strcpy(dest + strlen(dest), src);
  return dest;
}

static inline int abs(int value) { return (value >= 0) ? value : -value; }

__attribute__((noreturn)) static inline void _exit(int code) {
  register long a0 asm("a0") = code;

  asm volatile("r%=: wfi \nj r%=\n" ::"r"(a0));
  __builtin_unreachable();
}

__attribute__((noreturn)) static inline void __wrap_exit(int code) { _exit(code); }

static inline unsigned utoa10(unsigned v, char *out) {
  // returns number of bytes written (no NUL)
  char tmp[10];
  unsigned n = 0;
  do {
    tmp[n++] = (char)('0' + (v % 10u));
    v /= 10u;
  } while (v && n < sizeof(tmp));

  // reverse into out
  for (unsigned i = 0; i < n; ++i) out[i] = tmp[n - 1u - i];
  return n;
}

__attribute__((noreturn)) void __assert_func(const char *file, int line, const char *func, const char *expr) {
  const int fd = 1;

  // "assertion failed: "
  {
    static const char p[] = "assertion failed: ";
    (void)write(fd, p, sizeof(p) - 1);
  }

  // expr
  if (expr) (void)write(fd, expr, strlen(expr));
  else {
    static const char u[] = "(null)";
    (void)write(fd, u, sizeof(u) - 1);
  }

  // " @ "
  {
    static const char at[] = " @ ";
    (void)write(fd, at, sizeof(at) - 1);
  }

  // file
  if (file) (void)write(fd, file, strlen(file));
  else {
    static const char u[] = "(null)";
    (void)write(fd, u, sizeof(u) - 1);
  }

  // ":"
  {
    static const char c[] = ":";
    (void)write(fd, c, sizeof(c) - 1);
  }

  // line
  {
    char buf[16];
    unsigned n = utoa10((line < 0) ? 0u : (unsigned)line, buf);
    (void)write(fd, buf, n);
  }

  // " in "
  {
    static const char in[] = " in ";
    (void)write(fd, in, sizeof(in) - 1);
  }

  // func
  if (func) (void)write(fd, func, strlen(func));
  else {
    static const char u[] = "(null)";
    (void)write(fd, u, sizeof(u) - 1);
  }

  // "\n"
  {
    static const char nl[] = "\n";
    (void)write(fd, nl, sizeof(nl) - 1);
  }
  _exit(-1);
  // Stop execution: spin forever (or replace with your exit/trap syscall).
  for (;;) {
    __asm__ volatile("wfi");
  }
}
#ifdef __cplusplus
}
#endif
