#include "renderer.hpp"
#include <QPainter>

CursedCanvas::CursedCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  using Size = pepp::core::Size<i16>;
  using Pt = pepp::core::Point<i16>;
  // Create a semi-random assortment of same-sized rectangles to render.
  auto eight = Size{8, 8};
  _properties[_spatial_map.try_add(Rectangle{Pt{10, 20}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{30, 20}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{40, 20}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{50, 20}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{10, 30}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{30, 50}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{40, 80}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{50, 90}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{150, 90}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{250, 90}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{70, 70}, eight}).value()] = nullptr;
  _properties[_spatial_map.try_add(Rectangle{Pt{70, 250}, eight}).value()] = nullptr;
  // Initalize viewport to 0,0
  _top_left = Pt{0, 0};
}

void CursedCanvas::paint(QPainter *painter) {
  // Paint the background.
  QBrush brush(QColor(0x00, 0x74, 0x20));
  painter->setBrush(brush);
  painter->drawRect(0, 0, size().width(), size().height());

  // Determine the size of the viewport in grid coordinates.
  auto screen_viewport = QRectF(0, 0, size().width(), size().height());
  auto grid_viewport = screen_to_grid(screen_viewport);

  // Make rectangles a different color.
  painter->setBrush(QBrush(QColor(255, 0, 0)));
  for (const auto &[idx, rect] : _spatial_map) {
    auto props = _properties.find(idx);
    // Skip paiting rectangles that are outside the viewport.
    if (!pepp::core::intersects(grid_viewport, rect)) continue;
    else if (props == _properties.end()) continue;
    paint_one(painter, rect, props->second);
  }
}

void CursedCanvas::paint_one(QPainter *painter, Rectangle rect, void *props) {
  // Convert our absolute grid coordinates to screen coordinates.
  auto screen_rect = grid_to_screen(rect);
  // If we actually had props, we would use them to make decisions about how to paint.
  // e.g., do I copy one of the NAND/NOR images into this rectangle, or do I draw a solid color?
  painter->drawRect(screen_rect);
}

QRectF CursedCanvas::grid_to_screen(Rectangle rect) {
  const float offset_x = rect.top_left().x() - _top_left.x();
  const float offset_y = rect.top_left().y() - _top_left.y();
  const float width = rect.width();
  const float height = rect.height();

  return QRectF(offset_x * grid_to_px, offset_y * grid_to_px, width * grid_to_px, height * grid_to_px);
}

CursedCanvas::Rectangle CursedCanvas::screen_to_grid(QRectF rect) {
  const i16 x = static_cast<i16>(rect.x() / grid_to_px) + _top_left.x();
  const i16 y = static_cast<i16>(rect.y() / grid_to_px) + _top_left.y();
  const i16 width = static_cast<i16>(rect.width() / grid_to_px);
  const i16 height = static_cast<i16>(rect.height() / grid_to_px);
  return Rectangle{pepp::core::Point<i16>{x, y}, pepp::core::Size<i16>{width, height}};
}
