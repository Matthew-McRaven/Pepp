#pragma once
#include <QQuickPaintedItem>
#include "core/math/geom/rectangle.hpp"
class CursedCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCanvas)

public:
  CursedCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;

private:
  using Rectangle = pepp::core::Rectangle<i16>;
  void paint_one(QPainter *painter, Rectangle rect, void *props);
  QRectF grid_to_screen(Rectangle rect);
  std::vector<Rectangle> _rects;
  pepp::core::Point<i16> _top_left;
};
