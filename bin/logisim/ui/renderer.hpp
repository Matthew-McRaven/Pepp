#pragma once
#include <QQuickPaintedItem>
#include "core/math/geom/rectangle.hpp"

class CursedCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCanvas)
  Q_PROPERTY(float contentWidth READ contentWidth NOTIFY xBoundsChanged FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight NOTIFY yBoundsChanged FINAL)
  Q_PROPERTY(float originX READ originX WRITE setOriginX NOTIFY originChanged FINAL)
  Q_PROPERTY(float originY READ originY WRITE setOriginY NOTIFY originChanged FINAL)
public:
  CursedCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
  float maxX() const { return 1200; }
  float maxY() const { return 2000; }
  float contentWidth() const { return maxX(); }
  float contentHeight() const { return maxY(); }
  float originX() const { return _top_left.x() * grid_to_px; }
  float originY() const { return _top_left.y() * grid_to_px; }
  void setOriginX(float x) {
    _top_left = {static_cast<i16>(x / grid_to_px), _top_left.y()};
    emit originChanged();
    update();
  }
  void setOriginY(float y) {
    _top_left = {_top_left.x(), static_cast<i16>(y / grid_to_px)};
    emit originChanged();
    update();
  }
signals:
  void xBoundsChanged();
  void yBoundsChanged();
  void originChanged();

private:
  const float grid_to_px = 4.0f;
  using Rectangle = pepp::core::Rectangle<i16>;
  void paint_one(QPainter *painter, Rectangle rect, void *props);
  QRectF grid_to_screen(Rectangle rect);
  Rectangle screen_to_grid(QRectF rect);
  std::vector<Rectangle> _rects;
  pepp::core::Point<i16> _top_left;
};
