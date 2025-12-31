
#define REG (*(volatile unsigned long *)0xF0000010UL)

__attribute__((used, retain)) void hello_write(long value) { REG = value; }
__attribute__((used, retain)) long hello_read() { return REG; }

int main() {
  return 666;
}
