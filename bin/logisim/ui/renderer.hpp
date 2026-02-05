#pragma once
#include <QQuickPaintedItem>
#include "core/math/geom/rectangle.hpp"

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class CursedCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCanvas)
  // Sizes in "screen" coordinates
  Q_PROPERTY(float contentWidth READ contentWidth NOTIFY xBoundsChanged FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight NOTIFY yBoundsChanged FINAL)
  // In "screen" coordinates (e.g., pixels according to our containing Flickable)
  Q_PROPERTY(float originX READ originX WRITE setOriginX NOTIFY originChanged FINAL)
  Q_PROPERTY(float originY READ originY WRITE setOriginY NOTIFY originChanged FINAL)
public:
  CursedCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;
  // TODO: determine min/max bounds based on contained rectangles, then add padding around the edges.
  float contentWidth() const { return 1200; }
  float contentHeight() const { return 2000; }

  // The top-left corner, as measured in "screen" coordinates
  float originX() const { return _top_left.x() * grid_to_px; }
  float originY() const { return _top_left.y() * grid_to_px; }
  // Compute grid coordinates from screen coordinates
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
  // Magic constant to convert from grid coordinates to screen coordinates
  const float grid_to_px = 4.0f;
  // One of the classes from my geometry library. See core/math/geom
  using Rectangle = pepp::core::Rectangle<i16>;
  // Helepr for painting a single rect that has already "passed" the clipping test.
  void paint_one(QPainter *painter, Rectangle rect, void *props);
  QRectF grid_to_screen(Rectangle rect);
  Rectangle screen_to_grid(QRectF rect);

  // The things we want to render
  std::vector<Rectangle> _rects;
  // Top-left corner of the viewport in grid coordinates
  pepp::core::Point<i16> _top_left;
};
