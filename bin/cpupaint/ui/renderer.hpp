#pragma once
#include <QPixmap>
#include <QQuickPaintedItem>
#include "shapes_one.hpp"

using Geometry = std::variant<QRect, QPolygon, QLine, Arrow>;
struct Item {
  Geometry geom;
  QColor bg{0, 0, 0, 255}, fg{0, 0, 0, 255};
  bool enabled = true;
};

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class CursedCPUCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCPUCanvas)
  // Sizes in "screen" coordinates
  Q_PROPERTY(float contentWidth READ contentWidth CONSTANT FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight CONSTANT FINAL)
public:
  CursedCPUCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;

  float contentWidth() const { return 900; }
  float contentHeight() const { return 1050; }

private:
  std::array<QPixmap, 4> _arrows;
  std::vector<Item> _geom;
};
