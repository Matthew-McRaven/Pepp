#include "./native_libc.h"

long syscall1(long n, long arg0) {
  register long a0 __asm__("a0") = arg0;
  register long syscall_id __asm__("a7") = n;

  __asm__ volatile("scall" : "+r"(a0) : "r"(syscall_id));

  return a0;
}

__attribute__((used, retain)) long start() {
  syscall1(500, 1234567);
  return 1;
}
__attribute__((used, retain)) void preempt(int arg) { write(1, "Hello World!", arg); }

int main() {
  syscall1(500, 1234567);
  return 666;
}
