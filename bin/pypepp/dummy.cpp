#include <nanobind/nanobind.h>

int add(int a, int b) { return a + b; }

NB_MODULE(pypepp_native, m) { m.def("add", &add); }
