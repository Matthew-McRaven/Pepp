#include "graphiccanvas.h"
#include <QPainter>
#include <QSvgRenderer>

GraphicCanvas::GraphicCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    _rects.emplace_back(block_size, block_size, block_size, block_size);
    _rects.emplace_back(block_size * 2, block_size * 3, block_size, block_size);
    _rects.emplace_back(block_size * 3, block_size * 4, block_size, block_size);

    cacheImages(":/and");
    cacheImages(":/or");
    cacheImages(":/inverter");
    cacheImages(":/nand");
    cacheImages(":/nor");
    cacheImages(":/xor");
}

void GraphicCanvas::cacheImages(const QString &source)
{
    //  Pre-render SVG files so that paint is not slowed down
    QSvgRenderer renderer(source);
    renderer.setAspectRatioMode(Qt::KeepAspectRatio);

    if (renderer.isValid()) {
        QImage image(block_size * 100, block_size * 100, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        // Get QPainter that paints to the image
        QPainter painter(&image);
        renderer.render(&painter);

        _svgs.emplace_back(image);
    }
}

void GraphicCanvas::paint(QPainter *painter)
{
    // Paint the background.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(0, 0, size().width(), size().height());

    // Determine the size of the viewport in grid coordinates.
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height());
    const auto grid_viewport = screen_to_grid(screen_viewport);

    // Make rectangles a different color.
    painter->setBrush(QBrush(QColor(255, 0, 0)));
    int i = 0;
    for (const auto &rect : _rects) {
        // Skip paiting rectangles that are outside the viewport.
        //if (!pepp::core::intersects(grid_viewport, rect)) continue;
        if (!grid_viewport.intersects(rect))
            continue;

        // Convert our absolute grid coordinates to screen coordinates.
        auto screen_rect = grid_to_screen(rect);
        // If we actually had props, we would use them to make decisions about how to paint.
        // e.g., do I copy one of the NAND/NOR images into this rectangle, or do I draw a solid color?
        //painter->drawRect(screen_rect);
        painter->drawImage(screen_rect, _svgs[++i % _svgs.size()]);
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
