#include "placeholder.hpp"
#include "random"
int placeholder() {
  if (rand() % 2)
    return 0;
  else
    return 1;
}