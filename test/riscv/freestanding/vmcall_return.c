__attribute__((used, retain)) const char *hello() { return "Hello World!"; }

static struct Data {
  int val1;
  int val2;
  float f1;
} data = {.val1 = 1, .val2 = 2, .f1 = 3.0f};

__attribute__((used, retain)) struct Data *structs() { return &data; }

int main() { return 666; }
