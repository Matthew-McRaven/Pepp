#include "renderer.hpp"
#include <QPainter>

CursedCanvas::CursedCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  using Size = pepp::core::Size<i16>;
  using Pt = pepp::core::Point<i16>;
  auto eight = Size{8, 8};
  _rects.emplace_back(Rectangle{Pt{10, 20}, eight});
  _rects.emplace_back(Rectangle{Pt{30, 20}, eight});
  _rects.emplace_back(Rectangle{Pt{40, 20}, eight});
  _rects.emplace_back(Rectangle{Pt{50, 20}, eight});

  _rects.emplace_back(Rectangle{Pt{10, 30}, eight});
  _rects.emplace_back(Rectangle{Pt{30, 50}, eight});
  _rects.emplace_back(Rectangle{Pt{40, 80}, eight});
  _rects.emplace_back(Rectangle{Pt{50, 90}, eight});
  _rects.emplace_back(Rectangle{Pt{150, 90}, eight});
  _rects.emplace_back(Rectangle{Pt{250, 90}, eight});
  _top_left = Pt{0, 0};
}

void CursedCanvas::paint(QPainter *painter) {
  QBrush brush(QColor(0x00, 0x74, 0x20));
  painter->setBrush(brush);
  painter->drawRect(0, 0, size().width(), size().height());
  painter->setBrush(QBrush(QColor(255, 0, 0)));

  auto screen_viewport = QRectF(0, 0, size().width(), size().height());
  auto grid_viewport = screen_to_grid(screen_viewport);

  for (const auto &rect : _rects) {
    if (!pepp::core::intersects(grid_viewport, rect)) continue;

    painter->save();
    paint_one(painter, rect, nullptr);
    painter->restore();
  }
}

void CursedCanvas::paint_one(QPainter *painter, Rectangle rect, void *props) {
  auto screen_rect = grid_to_screen(rect);
  painter->drawRect(screen_rect);
}

QRectF CursedCanvas::grid_to_screen(Rectangle rect) {
  const auto offset_x = rect.top_left().x() - _top_left.x();
  const auto offset_y = rect.top_left().y() - _top_left.y();
  const auto width = rect.width();
  const auto height = rect.height();

  return QRectF(offset_x * grid_to_px, offset_y * grid_to_px, width * grid_to_px, height * grid_to_px);
}

CursedCanvas::Rectangle CursedCanvas::screen_to_grid(QRectF rect) {
  const i16 x = static_cast<i16>(rect.x() / grid_to_px) + _top_left.x();
  const i16 y = static_cast<i16>(rect.y() / grid_to_px) + _top_left.y();
  const i16 width = static_cast<i16>(rect.width() / grid_to_px);
  const i16 height = static_cast<i16>(rect.height() / grid_to_px);
  return Rectangle{pepp::core::Point<i16>{x, y}, pepp::core::Size<i16>{width, height}};
}
