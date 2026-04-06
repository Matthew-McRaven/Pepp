#pragma once

#include <QPixmap>
#include "../schematic/orient.hpp"

// Store a pixmap which has been aligned to the four cardinal directions. This allows for efficient retrieval of the
// correctly rotated pixmap for a given direction, without needing to rotate on-the-fly during rendering.
// Since these rotation are always by 90 degrees, conversion between the pixmaps is lossless.
class PrerotatedPixmap {
public:
  static PrerotatedPixmap from_left(const QPixmap &pixmap);
  static PrerotatedPixmap from_right(const QPixmap &pixmap);
  static PrerotatedPixmap from_up(const QPixmap &pixmap);
  static PrerotatedPixmap from_down(const QPixmap &pixmap);
  static PrerotatedPixmap from(const QPixmap &pixmap, Direction dir);

  inline const QPixmap *left() const { return &_left; }
  inline const QPixmap *right() const { return &_right; }
  inline const QPixmap *up() const { return &_up; }
  inline const QPixmap *down() const { return &_down; }

  const QPixmap *get(Direction dir) const;

private:
  QPixmap _left;
  QPixmap _right;
  QPixmap _up;
  QPixmap _down;
};
