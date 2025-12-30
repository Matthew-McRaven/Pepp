#include "./native_libc.h"
__attribute__((used, retain, __visibility__("default"))) void hello() { write(1, "Hello World!", 12); }

int main() { return 666; }
