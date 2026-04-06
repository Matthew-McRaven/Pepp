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

Direction clockwise(Direction dir) {
  switch (dir) {
  case Direction::Left: return Direction::Up;
  case Direction::Right: return Direction::Down;
  case Direction::Up: return Direction::Right;
  case Direction::Down: return Direction::Left;
  }
  PEPP_UNREACHABLE();
}

Direction counter_clockwise(Direction dir) {
  switch (dir) {
  case Direction::Left: return Direction::Down;
  case Direction::Right: return Direction::Up;
  case Direction::Up: return Direction::Left;
  case Direction::Down: return Direction::Right;
  }
  PEPP_UNREACHABLE();
}

Direction from_angle(i16 angle) {
  switch (angle % 360) {
  case 0: return Direction::Right;
  case 90: return Direction::Down;
  case 180: return Direction::Left;
  case 270: return Direction::Up;
  }
  throw std::invalid_argument("Angle must be a multiple of 90");
}
