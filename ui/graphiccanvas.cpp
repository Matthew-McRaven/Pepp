#include "graphiccanvas.h"
#include <QPainter>

GraphicCanvas::GraphicCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    _rects.emplace_back(5, 15, 10, 10);
    _rects.emplace_back(15, 25, 10, 10);
}

void GraphicCanvas::paint(QPainter *painter)
{
    // Paint the background.
    //QBrush brush(QColor(0x00, 0x74, 0x20));
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(0, 0, size().width(), size().height());

    // Determine the size of the viewport in grid coordinates.
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height());
    const auto grid_viewport = screen_to_grid(screen_viewport);

    // Make rectangles a different color.
    painter->setBrush(QBrush(QColor(255, 0, 0)));
    for (const auto &rect : _rects) {
        // Skip paiting rectangles that are outside the viewport.
        //if (!pepp::core::intersects(grid_viewport, rect)) continue;
        if (!grid_viewport.intersects(rect))
            continue;

        // Convert our absolute grid coordinates to screen coordinates.
        auto screen_rect = grid_to_screen(rect);
        // If we actually had props, we would use them to make decisions about how to paint.
        // e.g., do I copy one of the NAND/NOR images into this rectangle, or do I draw a solid color?
        painter->drawRect(screen_rect);
    }
}

QRectF GraphicCanvas::grid_to_screen(QRectF rect)
{
    const float offset_x = rect.x() - _top_left.x();
    const float offset_y = rect.y() - _top_left.y();
    const float width = rect.width();
    const float height = rect.height();

    return QRectF(offset_x * grid_to_px,
                  offset_y * grid_to_px,
                  width * grid_to_px,
                  height * grid_to_px);
}

QRectF GraphicCanvas::screen_to_grid(QRectF rect)
{
    const float x = rect.x() / grid_to_px + _top_left.x();
    const float y = rect.y() / grid_to_px + _top_left.y();
    const float width = rect.width() / grid_to_px;
    const float height = rect.height() / grid_to_px;

    return QRectF{x, y, width, height};
}
