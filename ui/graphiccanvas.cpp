#include "graphiccanvas.hpp"
#include <QPainter>
#include <QSvgRenderer>

#include "diagramdatamodel.hpp"

GraphicCanvas::GraphicCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    //  Enable mouse
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true); // Enable hover events if needed

    setAntialiasing(true);

    //  Allow drag and drop
    //setAcceptDrops(true);

    //  Create background gid
    cacheBackground();

    //  Create image copies for later painting
    cacheImages(":/and");
    cacheImages(":/or");
    cacheImages(":/inverter");
    cacheImages(":/nand");
    cacheImages(":/nor");
    cacheImages(":/xor");

    //  The data model is initialized after construction.
    //  Trigger test data loading once model is setup using event
    QObject::connect(this, &GraphicCanvas::modelChanged, this, &GraphicCanvas::updateData);
}

void GraphicCanvas::updateData()
{
    //  Trigger repaint on data model updates
    //  Model must exist before it can be connected.
    QObject::connect(_model, &DiagramDataModel::dataChanged, this, &GraphicCanvas::updateCell);

    //  Test data-remove below in prod
    static std::array<QString, 6> lookup{"AND Gate",
                                         "OR Gate",
                                         "Inverter",
                                         "NAND Gate",
                                         "NOR Gate",
                                         "XOR Gate"};
    if (_model == nullptr)
        return;

    //  Loop to create blocks. Fill Full Grid
    const int rows = 10;
    const int cols = 10;

    for (auto i = 1; i < cols; ++i) {
        for (auto j = 0; j < rows; ++j) {
            QRect r{block_size * i, block_size * j, block_size, block_size};

            auto index = _model->index(i, j);
            DiagramProperties *data = _model->createItem(index);

            //  Add block data
            data->setName(lookup[i % _svgs.size()]);
            data->setRectangle({i, j, 1, 1});
            data->setType(i % _svgs.size());
            data->setOrientation(90 * j);

            insertImage(r, data);
        }
    }
    //  End of test data
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

void GraphicCanvas::cacheBackground()
{
    _background.fill(Qt::transparent);
    QPainter painter(&_background);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const auto width = _background.width();
    const auto height = _background.height();

    QPen pen;
    pen.setWidth(1);
    QList<qreal> dashes;
    QLine line;

    pen.setColor(QColorConstants::Svg::gainsboro);
    dashes << 1 << 3;
    pen.setDashPattern(dashes);
    painter.setPen(pen);

    //  Horizontal minor lines
    line.setLine(height / 4, 0, height / 4, width);
    painter.drawLine(line);
    line.setLine(height * .75, 0, height * .75, width);
    painter.drawLine(line);

    //  Vertical minor lines
    line.setLine(0, width / 4, height, width / 4);
    painter.drawLine(line);
    line.setLine(0, width * .75, height, width * .75);
    painter.drawLine(line);

    //  Horizontal mid lines
    dashes.clear();
    dashes << 1 << 2;
    pen.setDashPattern(dashes);
    pen.setColor(QColorConstants::Svg::cornflowerblue);
    painter.setPen(pen);
    line.setLine(height / 2, 0, height / 2, width);
    painter.drawLine(line);

    //  Vertical mid lines
    line.setLine(0, width / 2, height, width / 2);
    painter.drawLine(line);

    //  Major lines
    pen.setWidth(grid_to_px);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColorConstants::Svg::blue);
    painter.setPen(pen);
    line.setLine(height - 1, 0, height - 1, width - 1);
    painter.drawLine(line);

    //  Vertical mid lines
    line.setLine(0, width - 1, height - 1, width - 1);
    painter.drawLine(line);
}

void GraphicCanvas::paint(QPainter *painter)
{
    // Paint the background.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(0, 0, size().width(), size().height());

    //  Set scaling
    painter->scale(_currentZoom, _currentZoom);

    //  Determine the size of the viewport in grid coordinates.
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height());

    //  Clip painter to just visible area (including scrollbar)
    painter->setClipRect(screen_viewport);

    //  Use grid coordindates for checking rectangles
    const auto grid_viewport = screen_to_grid(screen_viewport);

    const qint32 col = static_cast<qint32>(grid_viewport.height()) / block_size + 1;
    const qint32 row = static_cast<qint32>(grid_viewport.width()) / block_size + 1;
    const qint32 viewSize = block_size * grid_to_px;
    QRect currentBlock{0, 0, viewSize, viewSize};

    for (int x = 0; x < col; ++x) {
        for (int y = 0; y < row; ++y) {
            painter->drawPixmap(currentBlock, _background);
            currentBlock.translate(viewSize, 0);
        }
        //  Reset to next column first row
        currentBlock.translate(-viewSize * row, viewSize);
    }

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
    QPixmap *image = nullptr;

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

void GraphicCanvas::updateCell(const QModelIndex &from, const QModelIndex &to)
{
    //  Cannot assume that upper right was passed first. Get absolute dimensions
    //  Note, coordinates are in columns and rows, not grid or screen
    const int x = std::min(from.row(), to.row());
    const int y = std::min(from.column(), to.column());
    //  Note, cell has implicit width of 1
    const int height = std::abs(from.row() - to.row()) + 1;
    const int width = std::abs(from.column() - to.column() + 1);

    //convert to screen coordinates
    QRectF rect{x * block_size * grid_to_px,
                y * block_size * grid_to_px,
                height * block_size * grid_to_px,
                width * block_size * grid_to_px};

    //  Update expects integer coordinates
    update(rect.toRect());
}

//  Mouse events - Comment out unused events for now
/*void GraphicCanvas::mouseDoubleClickEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event) {}*/

void GraphicCanvas::mousePressEvent(QMouseEvent *event)
{
    //  Determine the size of the viewport in grid coordinates.
    //  Exclude scrollbar from view area otherwise, we will paint on scrollbars
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height());

    //  If point is not inside grid, let parent handle event and leave.
    //  Note, all values in screen coordinates
    if (!screen_viewport.contains(event->position())) {
        event->setAccepted(false);
        return;
    }

    //  Mouse location in grid coordinates to
    //  to determine rectangle hit.
    const auto point = screen_to_grid(event->position());

    //  See if existing item was clicked
    if (setSelected(point)) {
        //  Another item was selected
        event->setAccepted(true);
        return;
    }

    //  No template is selected, just return
    if (_template == nullptr) {
        event->setAccepted(false);
        return;
    }

    //  If we get here, we have a new item. Insert into canvas
    const int col = point.x() / block_size;
    const int row = point.y() / block_size;
    QRect r{block_size * col, block_size * row, block_size, block_size};
    const auto index = _model->index(col, row);
    DiagramProperties *data = _model->createItem(index);

    //  Add block data
    data->setName(_template->name());
    data->setRectangle({col, row, 1, 1});
    data->setType(_template->key());

    insertImage(r, data);
    _model->setData(index, true, DiagramProperty::Role::Selected);
    event->setAccepted(true);
}

bool GraphicCanvas::setSelected(const QPoint point)
{
    bool found{false};

    //  See if existing item was clicked and clear selection
    for (auto &[rect, props] : _rects) {
        // Skip painting rectangles that are outside the viewport.
        if (!rect.contains(point)) {
            if (props->selected()) {
                //  Item was previously selected, clear old outline
                //  Set through datamodel so that other controls see change
                const auto index = _model->index(props->rectangle().x(), props->rectangle().y());
                _model->setData(index, false, DiagramProperty::Role::Selected);

                //  Update unselected rectangle
                update();
            }
            continue;
        }

        // Item exists and is selected, update view
        //  Set through view so that other controls see change
        const auto index = _model->index(props->rectangle().x(), props->rectangle().y());
        _model->setData(index, true, DiagramProperty::Role::Selected);

        //  Update current rectangle
        update();

        found = true;
    }
    return found;
}

/*void GraphicCanvas::mouseReleaseEvent(QMouseEvent *event) {}

void GraphicCanvas::mouseUngrabEvent() {}

void GraphicCanvas::hoverEnterEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverLeaveEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverMoveEvent(QHoverEvent *event) {}
*/

void GraphicCanvas::wheelEvent(QWheelEvent *event)
{
    // A positive value for angleDelta().y() indicates the wheel was rotated
    // forwards/up, a negative value indicates backwards/down.
    const QPoint angleDelta = event->angleDelta();

    //  No mouse movement, just return
    if (angleDelta.y() == 0) {
        // No mouse movement, ignore event
        event->accept();
        return;
    }

    //  See if shift, alt, or control keys are pressed
    const auto modifier = event->modifiers();

    if (modifier == Qt::NoModifier) {
        float y = 0.0;
        if (angleDelta.y() > 0) {
            // Perform action for scrolling up
            y = std::max(0.0, originY() - 100.0);
        } else if (angleDelta.y() < 0) {
            // Perform action for scrolling down
            y = std::min(contentHeight(), originY() + 100);
        }

        //  Update screen
        setOriginY(y);
    }
    // Process horizontal scrolling if available
    // Normal h-scrolling uses angleDelta.x() != 0. Current mouse does not
    // Trigger h-scrolling. Use normal scrolling with shift key
    if (modifier == Qt::ShiftModifier) {
        float x = 0.0;
        if (angleDelta.y() < 0) {
            // Perform action for scrolling left
            x = std::max(0.0, originX() - 100.0);
        } else if (angleDelta.y() > 0) {
            // Perform action for scrolling right
            x = std::min(contentWidth(), originX() + 100);
        }

        //  Update screen
        setOriginX(x);
    }

    //  Control + mouse wheel triggeres zoom operations
    if (modifier == Qt::ControlModifier) {
        float x = 0.0;
        if (angleDelta.y() > 0) {
            // Perform action for scrolling up
            setZoom(1);
        } else if (angleDelta.y() < 0) {
            // Perform action for zoom in
            setZoom(-1);
        }

        //  Refresh screen on zoom
        update();
    }

    // Accept the event to stop it from propagating to parent items/widgets.
    // If ignored, a parent item might handle the event (e.g., a surrounding Flickable).
    event->accept();
}

void GraphicCanvas::setZoom(qint8 change)
{
    qreal newZoom = 0;
    if (change == 0)
        return;
    else if (change > 0)
        newZoom = std::min(_maxScale, _currentZoom + .25);
    else
        newZoom = std::max(_minScale, _currentZoom - .25);

    //  Apply rounding
    _currentZoom = newZoom; //static_cast<int>(newZoom * 100) / 100.0;
    //grid_to_px = _pixel * _currentZoom;
    qDebug() << "new zoom: " << newZoom;
}

void GraphicCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
    qDebug() << "dragEnterEvent";
}
void GraphicCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->ignore();
    qDebug() << "dragLeaveEvent";
}

void GraphicCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    event->ignore();
    qDebug() << "dragMoveEvent";
}

void GraphicCanvas::dropEvent(QDropEvent *event)
{
    event->ignore();
    qDebug() << "dropEvent";
}
