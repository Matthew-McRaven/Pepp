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
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height()) - _scrollbarWidth;

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
            qDebug() << "Wheel moved in";
            // Perform action for scrolling up
            x = std::max(0.0, originX() - 100.0);
        } else if (angleDelta.y() < 0) {
            qDebug() << "Wheel moved out";
            // Perform action for scrolling down
            x = std::min(contentWidth(), originX() + 100);
        }

        //  Update screen
        if (x != 0.0)
            setOriginX(x);
    }
    if (angleDelta.x() != 0) {
        qDebug() << "Horizontal wheel movement detected:" << angleDelta.x();
    }

    // Accept the event to stop it from propagating to parent items/widgets.
    // If ignored, a parent item might handle the event (e.g., a surrounding Flickable).
    event->accept();
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
