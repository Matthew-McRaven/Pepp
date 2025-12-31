#include "./native_libc.h"
static void (*other_exit)() = (void (*)())0xF0000000;

int main() {
  other_exit();
  _exit(1234);
}
