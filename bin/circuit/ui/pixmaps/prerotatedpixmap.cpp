#include "prerotatedpixmap.hpp"
#include <stdexcept>

PrerotatedPixmap PrerotatedPixmap::from_left(const QPixmap &pixmap) {
  PrerotatedPixmap result;
  result._left = pixmap;
  result._right = pixmap.transformed(QTransform().rotate(180));
  result._up = pixmap.transformed(QTransform().rotate(-90));
  result._down = pixmap.transformed(QTransform().rotate(90));
  return result;
}

PrerotatedPixmap PrerotatedPixmap::from_right(const QPixmap &pixmap) {
  PrerotatedPixmap result;
  result._right = pixmap;
  result._left = pixmap.transformed(QTransform().rotate(180));
  result._up = pixmap.transformed(QTransform().rotate(90));
  result._down = pixmap.transformed(QTransform().rotate(-90));
  return result;
}

PrerotatedPixmap PrerotatedPixmap::from_up(const QPixmap &pixmap) {
  PrerotatedPixmap result;
  result._left = pixmap.transformed(QTransform().rotate(90));
  result._right = pixmap.transformed(QTransform().rotate(-90));
  result._up = pixmap;
  result._down = pixmap.transformed(QTransform().rotate(180));
  return result;
}

PrerotatedPixmap PrerotatedPixmap::from_down(const QPixmap &pixmap) {
  PrerotatedPixmap result;
  result._down = pixmap;
  result._up = pixmap.transformed(QTransform().rotate(180));
  result._left = pixmap.transformed(QTransform().rotate(90));
  result._right = pixmap.transformed(QTransform().rotate(-90));
  return result;
}

PrerotatedPixmap PrerotatedPixmap::from(const QPixmap &pixmap, Direction dir) {
  switch (dir) {
  case Direction::Left: return from_left(pixmap);
  case Direction::Right: return from_right(pixmap);
  case Direction::Up: return from_up(pixmap);
  case Direction::Down: return from_down(pixmap);
  }
  throw std::invalid_argument("Invalid direction");
}

const QPixmap *PrerotatedPixmap::get(Direction dir) const {
  switch (dir) {
  case Direction::Left: return left();
  case Direction::Right: return right();
  case Direction::Up: return up();
  case Direction::Down: return down();
  default: throw std::invalid_argument("Invalid direction");
  }
}
