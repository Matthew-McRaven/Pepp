#include <float.h>

inline int kinda64(float val, double expectation) {
  return val >= expectation - FLT_EPSILON && val < expectation + FLT_EPSILON;
}

static struct {
  double sum;
  int counter;
  int sign;
} pi;

static double compute_more_pi() {
  pi.sum += pi.sign / (2.0 * pi.counter + 1.0);
  pi.counter++;
  pi.sign = -pi.sign;
  return 4.0 * pi.sum;
}
int main() {
  pi.sign = 1;
  if (!kinda64(compute_more_pi(), 4.0)) return 1;
  if (!kinda64(compute_more_pi(), 2.66666666666)) return 2;
  if (!kinda64(compute_more_pi(), 3.46666666666)) return 3;
  return 0;
}
