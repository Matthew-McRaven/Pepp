__attribute__((used, retain)) int pass_iarray(const int *data, unsigned size) {
  if (size != 3) return 0;
  if (data[0] != 1 || data[1] != 2 || data[2] != 3) return 0;
  return 1;
}

__attribute__((used, retain)) int pass_farray(const float *data, unsigned size) {
  if (size != 3) return 0;
  if (data[0] != 1.0f || data[1] != 2.0f || data[2] != 3.0f) return 0;
  return 1;
}

struct Data {
  int val1;
  int val2;
  float f1;
};
__attribute__((used, retain)) int pass_struct(const struct Data *data, unsigned size) {
  if (size != 3) return 0;
  if (data[0].val1 != 1 || data[0].val2 != 2 || data[0].f1 != 3.0f) return 0;
  if (data[1].val1 != 4 || data[1].val2 != 5 || data[1].f1 != 6.0f) return 0;
  if (data[2].val1 != 7 || data[2].val2 != 8 || data[2].f1 != 9.0f) return 0;
  return 1;
}

int main() { return 666; }
