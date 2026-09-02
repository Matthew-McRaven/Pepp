int do_syscall(int value) {
  register long a0 __asm__("a0") = value;
  register long syscall_id __asm__("a7") = 0;

  __asm__ volatile("ecall" : "+r"(a0) : "r"(syscall_id));
  return a0;
}
__attribute__((used, retain)) int mycall(int value) {
  if (value != 1) return 727;
  return do_syscall(value);
}

int main() { return 666; }
