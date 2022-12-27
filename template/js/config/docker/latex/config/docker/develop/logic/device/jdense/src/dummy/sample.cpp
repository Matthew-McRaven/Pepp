#include "sample.hpp"
#include <random>
double Test::foo() {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> dist(0, 10);
  return dist(e2);
}
