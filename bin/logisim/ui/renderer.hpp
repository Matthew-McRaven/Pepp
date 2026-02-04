#pragma once
#include <QQuickPaintedItem>
#include "core/math/geom/rectangle.hpp"
class CursedCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCanvas)
  Q_PROPERTY(float contentWidth READ contentWidth NOTIFY xBoundsChanged FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight NOTIFY yBoundsChanged FINAL)
public:
  CursedCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
  float maxX() const { return 600; }
  float maxY() const { return 2000; }
  float contentWidth() const { return maxX(); }
  float contentHeight() const { return maxY(); }
signals:
  void xBoundsChanged();
  void yBoundsChanged();

private:
  const float grid_to_px = 4.0f;
  using Rectangle = pepp::core::Rectangle<i16>;
  void paint_one(QPainter *painter, Rectangle rect, void *props);
  QRectF grid_to_screen(Rectangle rect);
  Rectangle screen_to_grid(QRectF rect);
  std::vector<Rectangle> _rects;
  pepp::core::Point<i16> _top_left;
};
