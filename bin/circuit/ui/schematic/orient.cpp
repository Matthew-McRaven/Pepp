#include "orient.hpp"
#include "core/macros.hpp"

bool parallel(Direction a, Direction b) {
  switch (a) {
  case Direction::Left: [[fallthrough]];
  case Direction::Right: return b == Direction::Left || b == Direction::Right;
  case Direction::Up: [[fallthrough]];
  case Direction::Down: return b == Direction::Up || b == Direction::Down;
  }
  PEPP_UNREACHABLE();
}
