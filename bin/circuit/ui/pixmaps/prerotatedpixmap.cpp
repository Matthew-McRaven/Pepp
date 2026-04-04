#include "prerotatedpixmap.hpp"
#include <stdexcept>

PrerotatedPixmap PrerotatedPixmap::from_left(const QPixmap &pixmap) {
  return from_rotations(pixmap, /*to_left=*/0, /*to_right=*/180,
                        /*to_up=*/90, /*to_down=*/270);
}

PrerotatedPixmap PrerotatedPixmap::from_right(const QPixmap &pixmap) {
  return from_rotations(pixmap, /*to_left=*/180, /*to_right=*/0,
                        /*to_up=*/270, /*to_down=*/90);
}

PrerotatedPixmap PrerotatedPixmap::from_up(const QPixmap &pixmap) {
  return from_rotations(pixmap, /*to_left=*/270, /*to_right=*/90,
                        /*to_up=*/0, /*to_down=*/180);
}

PrerotatedPixmap PrerotatedPixmap::from_down(const QPixmap &pixmap) {
  return from_rotations(pixmap, /*to_left=*/90, /*to_right=*/270,
                        /*to_up=*/180, /*to_down=*/0);
}

PrerotatedPixmap PrerotatedPixmap::from(const QPixmap &pixmap, Direction dir) {
  switch (dir) {
  case Direction::Left: return from_left(pixmap);
  case Direction::Right: return from_right(pixmap);
  case Direction::Up: return from_up(pixmap);
  case Direction::Down: return from_down(pixmap);
  }
  Q_UNREACHABLE();
}

const QPixmap &PrerotatedPixmap::get(Direction dir) const {
  switch (dir) {
  case Direction::Left: return left();
  case Direction::Right: return right();
  case Direction::Up: return up();
  case Direction::Down: return down();
  }
  Q_UNREACHABLE();
}

const QPixmap &PrerotatedPixmap::get(int rotation) const {
  switch (rotation % 360) {
  case 0: return right();
  case 90: return up();
  case 180: return left();
  case 270: return down();
  }
  throw std::invalid_argument("Rotation must be a multiple of 90");
  Q_UNREACHABLE();
}

QPixmap PrerotatedPixmap::rotated(const QPixmap &p, int degrees) {
  return degrees == 0 ? p : p.transformed(QTransform().rotate(degrees));
}

PrerotatedPixmap PrerotatedPixmap::from_rotations(const QPixmap &src, int to_left, int to_right, int to_up,
                                                  int to_down) {
  PrerotatedPixmap result;
  result._left = rotated(src, to_left);
  result._right = rotated(src, to_right);
  result._up = rotated(src, to_up);
  result._down = rotated(src, to_down);
  return result;
}
