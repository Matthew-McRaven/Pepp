int main() {
  __asm__("li t0, 1234");           // Load integer in T0
  __asm__("fcvt.d.w fa1, t0");      // Move integer from T0 to FA1 (64-bit fp)
  __asm__("li a3, 0xDEADB33F");     // Load integer in A3
  __asm__("li a7, 500");            // System call number 500
  __asm__(".word 0b1000011011011"); // Indicate F1 contains a 64-bit fp argument
  __asm__(".word 0b0000111011011"); // Indicate A3 contains a 64-bit unsigned argument
  __asm__("ecall");                 // Execute system call
  __asm__("ret");
}
