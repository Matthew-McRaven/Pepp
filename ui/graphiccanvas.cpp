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

    //  Create image copies for later painting
    cacheImages(":/and");
    cacheImages(":/or");
    cacheImages(":/inverter");
    cacheImages(":/nand");
    cacheImages(":/nor");
    cacheImages(":/xor");

    //  Loop to create blocks. Fill Full Grid
    const int rows = 10;
    const int cols = 10;

    for (auto i = 0; i < rows; ++i) {
        for (auto j = 0; j < cols; ++j) {
            QRect r{block_size * i, block_size * j, block_size, block_size};

            DiagramProperties *data = new DiagramProperties(this);
            data->setType(i % _svgs.size());
            data->setOrientation(90 * j);

            insertImage(r, data);
        }
    }
    /*QRect r1{block_size, block_size, block_size, block_size};
    QRect r2{block_size * 2, block_size * 3, block_size, block_size};
    QRect r3{block_size * 3, block_size * 4, block_size, block_size};

    DiagramProperties *data = new DiagramProperties(this);
    data->setType(DiagramType::Type::ANDGate); // + DiagramType::Type::TotalGates);
    data->setOrientation(90);
    insertImages(r1, data);
    //_rects.push_back({r1, data});

    data = new DiagramProperties(this);
    data->setType(DiagramType::Type::XORGate);
    data->setOrientation(180);
    insertImages(r2, data);
    //_rects.push_back({r2, data});

    data = new DiagramProperties(this);
    data->setType(DiagramType::Type::NANDGate);
    data->setOrientation(271); // Intentionally wrong for testing
    insertImages(r3, data);
    //_rects.push_back({r3, data});*/
}

void GraphicCanvas::insertImage(const QRect &rect, DiagramProperties *data)
{
    //  Keep track of canvas size
    _dimensions = _dimensions.united(rect);
    _rects.push_back({rect, data});
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
        /*painter.end();

        // Apply the new color - colors fill
        painter.begin(&image);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(image.rect(), QBrush(_highlight));
        painter.end();*/
        _svgs.emplace_back(image);

        //  Make copy of rotated image to speed up drawing
        _svgsBottom.emplaceBack(image.transformed(QTransform().rotate(90)));
        _svgsLeft.emplaceBack(image.transformed(QTransform().rotate(180)));
        _svgsTop.emplaceBack(image.transformed(QTransform().rotate(270)));
    }
}

void GraphicCanvas::paint(QPainter *painter)
{
    // Paint the background.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(0, 0, size().width(), size().height());

    //  Determine the size of the viewport in grid coordinates.
    //  Exclude scrollbar from view area otherwise, we will paint on scrollbars
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height()) - _scrollbarWidth;

    //  Clip painter to just visible area (including scrollbar)
    painter->setClipRect(screen_viewport);

    //  Use grid coordindates for checking rectangles
    const auto grid_viewport = screen_to_grid(screen_viewport);

    for (const auto &[rect, props] : _rects) {
        // Skip painting rectangles that are outside the viewport.
        if (grid_viewport.intersects(rect))
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
    QPixmap *image = nullptr; // = &_svgs[props.type()];

    //  Check state, and set outline if selected
    if (props.selected()) {
        painter->setPen(QPen(_highlight, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRect(screen_rect);
    }

    //  Get cached copy for drawing
    switch (props.orientation()) {
    case 90:
        image = &_svgsBottom[props.type()];
        break;
    case 180:
        image = &_svgsLeft[props.type()];
        break;
    case 270:
        image = &_svgsTop[props.type()];
        break;
    default:
        image = &_svgs[props.type()];
        break;
    }

    //  If image is not null, it can be output
    if (image)
        painter->drawPixmap(screen_rect.toRect(), *image);
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

//  Mouse events - Comment out unused events for now
/*void GraphicCanvas::mouseDoubleClickEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event) {}*/

void GraphicCanvas::mousePressEvent(QMouseEvent *event)
{
    //  Determine the size of the viewport in grid coordinates.
    //  Exclude scrollbar from view area otherwise, we will paint on scrollbars
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height()) - _scrollbarWidth;

    //  If point is not inside grid, let parent handle event and leave
    //  Note, all values in screen coordinates
    if (!screen_viewport.contains(event->position())) {
        event->setAccepted(false);
        return;
    }

    //  Mouse location in grid coordinates to
    //  to determine rectangle hit.
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

        event->setAccepted(true);
    }
}

/*void GraphicCanvas::mouseReleaseEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseUngrabEvent() {}

void GraphicCanvas::hoverEnterEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverLeaveEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverMoveEvent(QHoverEvent *event) {}
*/
