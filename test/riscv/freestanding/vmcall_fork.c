#include "./native_libc.h"
static int value = 0;

__attribute__((used, retain)) void hello() {
  if (value != 1) fail();
  value = 0;
  write(1, "Hello World!", 12);
}

__attribute__((used, retain)) int str(const char *arg) {
  if (strcmp(arg, "Hello") != 0) fail();
  return 1;
}

struct Data {
  int val1;
  int val2;
  float f1;
};
__attribute__((used, retain)) int structs(struct Data *data) {
  if (data->val1 != 1) fail();
  if (data->val2 != 2) fail();
  if (data->f1 != 3.0f) fail();
  return 2;
}

__attribute__((used, retain)) int ints(long i1, long i2, long i3) {
  if (i1 != 123) fail();
  if (i2 != 456) fail();
  if (i3 != 456) fail();
  return 3;
}

__attribute__((used, retain)) int fps(float f1, double d1) {
  if (f1 != 1.0f) fail();
  if (d1 != 2.0) fail();
  return 4;
}

int main() {
  value = 1;
  return 666;
}
