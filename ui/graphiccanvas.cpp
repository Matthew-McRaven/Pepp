#include "graphiccanvas.h"
#include <QPainter>
#include <QSvgRenderer>

GraphicCanvas::GraphicCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    //  Enable mouse
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true); // Enable hover events if needed

    setAntialiasing(true);
    QRect r1{block_size, block_size, block_size, block_size};
    QRect r2{block_size * 2, block_size * 3, block_size, block_size};
    QRect r3{block_size * 3, block_size * 4, block_size, block_size};

    DiagramProperties *data = new DiagramProperties(this);
    data->setType(DiagramType::Type::ANDGate); // + DiagramType::Type::TotalGates);
    _rects.push_back({r1, data});

    data = new DiagramProperties(this);
    data->setType(DiagramType::Type::XORGate);
    _rects.push_back({r2, data});

    data = new DiagramProperties(this);
    data->setType(DiagramType::Type::NANDGate);
    _rects.push_back({r3, data});

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
        QPixmap image(block_size * 10, block_size * 10);
        image.fill(Qt::transparent);

        // Get QPainter that paints to the image
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

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
    int i = 0;
    for (const auto &[rect, props] : _rects) {
        // Skip painting rectangles that are outside the viewport.
        if (!grid_viewport.intersects(rect))
            continue;

        paint_one(painter, rect, *props);
    }
}

void GraphicCanvas::paint_one(QPainter *painter, QRect rect, const DiagramProperties &props)
{
    // Convert our absolute grid coordinates to screen coordinates.
    auto screen_rect = grid_to_screen(rect).adjusted(2, 2, -3, -3);
    // In reality, each of these branches should be its own function/method.
    // If we actually had props, we would use them to make decisions about how to paint.
    // e.g., do I copy one of the NAND/NOR images into this rectangle, or do I draw a solid color?
    auto &image = _svgs[props.type()];

    //  Check state, and set outline if selected
    if (props.selected()) {
        painter->setPen(QPen(_highlight, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRect(screen_rect);
    }
    //painter->drawImage(screen_rect, image);
    painter->drawPixmap(screen_rect.toRect(), image);
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
    //.toRect();
}

QRectF GraphicCanvas::screen_to_grid(QRectF rect)
{
    const float x = rect.x() / grid_to_px + _top_left.x();
    const float y = rect.y() / grid_to_px + _top_left.y();
    const float width = rect.width() / grid_to_px;
    const float height = rect.height() / grid_to_px;

    return QRectF{x, y, width, height};
}

QPoint GraphicCanvas::screen_to_grid(QPointF point)
{
    const float x = point.x() / grid_to_px + _top_left.x();
    const float y = point.y() / grid_to_px + _top_left.y();

    return QPointF{x, y}.toPoint();
}

//  Mouse events
void GraphicCanvas::mouseDoubleClickEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event) {}

void GraphicCanvas::mousePressEvent(QMouseEvent *event)
{
    // Determine the mouse location in grid coordinates.
    const auto point = screen_to_grid(event->position());
    for (auto &[rect, props] : _rects) {
        // Skip painting rectangles that are outside the viewport.
        if (!rect.contains(point)) {
            if (props->selected()) {
                //  Item was previously selected, clear old outline
                props->setSelected(false);
                update(grid_to_screen(rect).toRect());
            }
            continue;
        }
        // Item is selected, update view
        props->setSelected(true);

        //  Update current rectangle
        update(grid_to_screen(rect).toRect());
    }
}

void GraphicCanvas::mouseReleaseEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseUngrabEvent() {}

void GraphicCanvas::hoverEnterEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverLeaveEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverMoveEvent(QHoverEvent *event) {}
