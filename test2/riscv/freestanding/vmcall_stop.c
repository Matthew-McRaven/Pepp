#include "./native_libc.h"
long syscall1(long n, long arg0) {
  register long a0 __asm__("a0") = arg0;
  register long syscall_id __asm__("a7") = n;

  __asm__ volatile("scall" : "+r"(a0) : "r"(syscall_id));

  return a0;
}
void return_fast1(long retval) {
  register long a0 __asm__("a0") = retval;

  __asm__ volatile(".insn i SYSTEM, 0, x0, x0, 0x7ff" ::"r"(a0));
  __builtin_unreachable();
}

__attribute__((used, retain)) long start() {
  syscall1(500, 1234567);
  return_fast1(1234);
  return 5678;
}
__attribute__((used, retain)) long preempt(int arg) {
  write(1, "Hello World!", arg);
  return_fast1(777);
}

int main() {
  syscall1(500, 1234567);
  return_fast1(777);
  return 666;
}
