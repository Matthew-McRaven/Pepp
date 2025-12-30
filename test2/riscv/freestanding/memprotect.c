static const int array[4] = {1, 2, 3, 4};
__attribute__((optimize("-O0"))) int main() {
  *(volatile int *)array = 1234;

  if (array[0] != 1234) return -1;
  return 666;
}
__attribute__((used, retain)) void write_to(char *dst) { *dst = 1; }
__attribute__((used, retain)) int read_from(char *dst) { return *dst; }
