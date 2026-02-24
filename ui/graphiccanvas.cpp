#include "graphiccanvas.hpp"
#include <QApplication> //  For startDragDistance()
#include <QCursor>
#include <QDrag>
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
    setFlag(QQuickItem::ItemAcceptsDrops, true);

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

    DiagramProperties *data = addDiagram(2, 2);
    if (data == nullptr)
        return;
    //QRect gridRect{0, 0, major_block_size, major_block_size};

    //auto index = _model->index(0, 0);
    //DiagramProperties *data = _model->createItem(index);

    //  Add block data
    data->setName(lookup[0]);
    //data->setRectangle({0, 0, 1, 2});
    //data->setGridRectangle(gridRect - _margin);
    data->setType(0);
    data->setOrientation(0);
    getImage(*data);

    //  Keep track of canvas size
    //_dimensions = _dimensions.united(gridRect);

    /*gridRect.moveTopLeft({2 * minor_block_size, 1 * minor_block_size});

    index = _model->index(2, 1);
    data = _model->createItem(index);

    //  Add block data
    data->setName(lookup[1]);
    data->setRectangle({2, 1, 2, 2});
    data->setGridRectangle(gridRect);
    data->setType(1);
    data->setOrientation(90);
    getImage(*data);

    gridRect.moveTopLeft({5 * minor_block_size, 3 * minor_block_size});

    index = _model->index(5, 3);
    data = _model->createItem(index);

    //  Add block data
    data->setName(lookup[1]);
    data->setRectangle({5, 3, 2, 2});
    data->setGridRectangle(gridRect);
    data->setType(1);
    data->setOrientation(180);
    getImage(*data);
    */
    /*for (auto i = 0; i < cols; ++i) {
        for (auto j = 0; j < rows; ++j) {
            QRect gridRect{minor_block_size * (i + (j % 2)),
                           minor_block_size * (j + (i % 2)),
                           major_block_size,
                           major_block_size};

            auto index = _model->index(i, j);
            DiagramProperties *data = _model->createItem(index);

            //  Add block data
            data->setName(lookup[i % _svgs.size()]);
            data->setRectangle({i, j, 2, 2});
            data->setGridRectangle(gridRect);
            data->setType(i % _svgs.size());
            data->setOrientation(90 * j);
            getImage(*data);

            //  Keep track of canvas size
            _dimensions = _dimensions.united(gridRect);
        }
    }*/
    //  End of test data
}

void GraphicCanvas::cacheImages(const QString &source)
{
    //  Pre-render SVG files so that paint is not slowed down
    QSvgRenderer renderer(source);
    renderer.setAspectRatioMode(Qt::KeepAspectRatio);

    if (renderer.isValid()) {
        int dim = _background.width() * grid_to_px;
        //qDebug() << "dim, width, widthMM, logicalDpiX" << dim << _background.width()
        //         << _background.widthMM() << _background.logicalDpiX();
        QPixmap image(dim, dim);
        image.fill(Qt::transparent);

        // Get QPainter that paints to the image
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        renderer.render(&painter);
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
    pen.setWidth(grid_to_px / 2);
    QList<qreal> dashes;
    QLine line;

    pen.setColor(QColorConstants::Svg::gainsboro);
    dashes << 1 << 6;
    pen.setDashPattern(dashes);
    painter.setPen(pen);

    //  Horizontal minor lines
    line.setLine(height / 4 - 1, 0, height / 4, width - 1);
    painter.drawLine(line);
    line.setLine(height * .75 - 1, 0, height * .75, width - 1);
    painter.drawLine(line);

    //  Vertical minor lines
    line.setLine(0, width / 4 - 1, height, width / 4 - 1);
    painter.drawLine(line);
    line.setLine(0, width * .75 - 1, height, width * .75 - 1);
    painter.drawLine(line);

    //  Horizontal mid lines
    dashes.clear();
    dashes << 1 << 3;
    pen.setDashPattern(dashes);
    pen.setColor(QColorConstants::Svg::cornflowerblue);
    painter.setPen(pen);
    line.setLine(height / 2 - 1, 0, height / 2 - 1, width);
    painter.drawLine(line);

    //  Vertical mid lines
    line.setLine(0, width / 2 - 1, height, width / 2 - 1);
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
    //  Determine the size of the viewport in grid coordinates.
    const auto screen_viewport = QRectF(0, 0, size().width(), size().height());

    // Paint the background.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(screen_viewport);

    //  Set scaling - affects clipping region and column/row count
    //  Other painting is unaffected
    painter->scale(_currentZoom, _currentZoom);

    //  Use grid coordindates for checking rectangles. Note,
    //  screen_to_grid returns scaled grid.
    const auto grid_viewport = screen_to_grid(screen_viewport);

    //  Number of columns/rows changes with zoom
    //  Note, first and last column may be partial.
    //  drawPixmap implicity uses scale, but other calculations do not.
    //  Add 3 since scaling indirectly affects number of rows and columns.
    //  Scaling currentBlock causes banding and overwriting.
    const qint32 row = grid_viewport.height() / minor_block_size + 3;
    const qint32 col = grid_viewport.width() / minor_block_size + 3;
    //qDebug() << "row: " << row << "col:" << col;

    //  Offset first cell if first row or column is cut off
    qreal cX = std::fmod(grid_viewport.x(), major_block_size) * grid_to_px * _currentZoom;
    qreal cY = std::fmod(grid_viewport.y(), major_block_size) * grid_to_px * _currentZoom;

    QRectF currentBlock{-cX, -cY, screen_block, screen_block};

    //  Background is always painted on major grid axis
    for (int x = 0; x < col; ++x) {
        for (int y = 0; y < row; ++y) {
            painter->drawPixmap(currentBlock.toRect(), _background);
            currentBlock.translate(0.0, screen_block);
        }
        //  Reset to next column first row
        currentBlock.translate(screen_block, -screen_block * row);
    }

    //  Diagrams are painted on minor grid axis
    for (const auto props : _model->cells()) {
        //for (const auto &[rect, props] : _rects) {
        // Skip painting rectangles that are outside the viewport.
        if (grid_viewport.intersects(props->gridRectangle()))
            paint_one(painter, props->gridRectangle(), *props);
    }
    //qDebug() << "grid_viewport: " << grid_viewport;
}

void GraphicCanvas::paint_one(QPainter *painter, QRect rect, DiagramProperties &props)
{
    // Convert our absolute grid coordinates to screen coordinates.
    // Grid is inset so that selection box appears inside current cell
    auto screen_rect = grid_to_screen(rect).adjusted(2, 2, -3, -3);

    //  Check state, and set outline if selected
    if (props.selected()) {
        painter->setPen(QPen(_highlight, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRect(screen_rect);
    }

    //  If image is not null, it can be output
    if (props.image() == nullptr)
        //  If image is null, then it's properties were reset, update image
        getImage(props);

    painter->drawPixmap(screen_rect.toRect(), *props.image());
}

DiagramProperties *GraphicCanvas::addDiagram(const int col, const int row)
{
    //  Center point may put diagram off of page, return if either index is negative.
    if (col < 0 || row < 0)
        return nullptr;

    //  Create index and check for data
    const auto newIndex = _model->index(col, row);
    DiagramProperties *data = _model->createItem(newIndex);

    //  Add block data
    setGrid(data, col, row);

    if (_template != nullptr) {
        data->setName(_template->name());
        data->setType(_template->key());
    }
    getImage(*data);

    //  Set select flag
    _model->setData(newIndex, true, DiagramProperty::Role::Selected);

    return data;
}

void GraphicCanvas::setGrid(DiagramProperties *data, const int col, const int row)
{
    //  Column and row represents center point, not top left
    QRect gridRect{minor_block_size * col - major_block_size / 2,
                   minor_block_size * row - major_block_size / 2,
                   major_block_size,
                   major_block_size};

    //  Track dimensions of canvas area. Affects scrollbars
    _dimensions = _dimensions.united(gridRect);

    //  Add block data
    data->setRectangle({col, row, 2, 2});
    data->setGridRectangle(gridRect - _margin);
}

void GraphicCanvas::getImage(DiagramProperties &props)
{
    QPixmap *image = nullptr;

    //  If type has not been selected, just return.
    if (props.type() == DiagramType::Invalid)
        return;

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
    }
    props.setImage(image);
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

    return QRectF{x / _currentZoom, y / _currentZoom, width / _currentZoom, height / _currentZoom};
}

QPoint GraphicCanvas::screen_to_grid(QPointF point)
{
    const float x = point.x() / grid_to_px + _top_left.x();
    const float y = point.y() / grid_to_px + _top_left.y();

    return QPointF{x / _currentZoom, y / _currentZoom}.toPoint();
}

const QPoint GraphicCanvas::grid_to_index(const QPoint point) const
{
    //  Images are stored by row and column.
    //  Due to integer math, items closer to next row or column are still in same column/row.
    //  Calculate rounding difference
    const int dx = (point.x() % minor_block_size) > (minor_block_size / 2) ? 1 : 0;
    const int dy = (point.y() % minor_block_size) > (minor_block_size / 2) ? 1 : 0;

    return QPoint{point.x() / minor_block_size + dx, point.y() / minor_block_size + dy};
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
    QRectF rect{x * minor_block_size * grid_to_px,
                y * minor_block_size * grid_to_px,
                height * major_block_size * grid_to_px,
                width * major_block_size * grid_to_px};

    //  Update expects integer coordinates
    update(rect.toRect());
}

//  Mouse events - Comment out unused events for now
/*void GraphicCanvas::mouseDoubleClickEvent(QMouseEvent *event) {}*/

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "MouseMove" << event;
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - _dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    //  Mouse location in grid coordinates to
    //  to determine rectangle hit.
    const auto point = screen_to_grid(event->position());

    //  See if existing item was clicked
    if (setSelected(point)) {
        startDrag(event->pos());

        //  Another item was selected
        event->setAccepted(true);

        return;
    }
}

void GraphicCanvas::mousePressEvent(QMouseEvent *event)
{
    //  Only handle left click
    if (event->button() != Qt::LeftButton)
        return;

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
    //qDebug() << event->pos() << event->position();

    //  Images are stored by row and column.
    //  Due to integer math, items closer to next row or column are still in same column/row.
    //  Calculate rounding difference
    const auto index = grid_to_index(point);

    //  See if existing item was clicked
    if (setSelected(point)) {
        _dragStartPosition = event->position();
        return;
    }

    //  No template is selected, just return
    if (_template == nullptr) {
        event->setAccepted(false);
        return;
    }

    //  If we get here, we have a new item. Insert into canvas
    //  Use coordinate as center point
    DiagramProperties *data = addDiagram(index.x(), index.y());

    //  If no data is returned, the column is invalid. Assume parent will handle
    event->setAccepted(data != nullptr ? true : false);
}

bool GraphicCanvas::setSelected(const QPoint point)
{
    bool found{false};

    //  See if existing item was clicked and clear selection
    for (const auto props : _model->cells()) {
        // Skip painting rectangles that are outside the viewport.
        if (!props->gridRectangle().contains(point)) {
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

        //  Item exists and is selected, update view
        //  Save current item for other actions
        //  Set through view so that other controls see change
        _currentItem = props;
        const auto index = _model->index(props->rectangle().x(), props->rectangle().y());
        _model->setData(index, true, DiagramProperty::Role::Selected);

        //  Update current rectangle
        update();

        found = true;
    }
    return found;
}

void GraphicCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "MouseRelease" << event;
}

/*
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

    float block = screen_block * _currentZoom;

    if (modifier == Qt::NoModifier) {
        float y = 0.0;
        if (angleDelta.y() > 0) {
            // Perform action for scrolling up
            y = std::max(0.0f, originY() - block);
        } else if (angleDelta.y() < 0) {
            // Perform action for scrolling down
            y = std::min(contentHeight(), originY() + block);
        }
        //qDebug() << "contentHeight():" << contentHeight() << "originY():" << originY() << "new Y"
        //         << y;

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
            x = std::max(0.0f, originX() - block);
        } else if (angleDelta.y() > 0) {
            // Perform action for scrolling right
            x = std::min(contentWidth(), originX() + block);
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
    _currentZoom = newZoom;
    qDebug() << "new zoom: " << newZoom;
}

void GraphicCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    //qDebug() << "dragEnterEvent";
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void GraphicCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->ignore();
    //qDebug() << "dragLeaveEvent";
}

void GraphicCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    qDebug() << "dragMoveEvent";
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void GraphicCanvas::dropEvent(QDropEvent *event)
{
    qDebug() << "dropEvent";
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        qint32 oldX, oldY;
        dataStream >> oldX >> oldY;

        //  Mouse location in grid coordinates to
        //  to determine rectangle hit.
        const auto point = screen_to_grid(event->position());

        //  Due to integer math, items closer to next row or column are still in same column/row.
        //  Calculate rounding difference
        const auto newPtIndex = grid_to_index(point);

        const auto oldIndex = _model->index(oldX, oldY);
        const auto newIndex = _model->index(newPtIndex.x(), newPtIndex.y());

        //  Update grid coordinates
        DiagramProperties *data = _model->item(oldIndex);
        setGrid(data, newPtIndex.x(), newPtIndex.y());

        //  Update model
        _model->move(oldIndex, newIndex);
        unsetCursor();

        update();

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void GraphicCanvas::startDrag(const QPoint point)
{
    _dragStartPosition = point;
    QDrag *drag = new QDrag(this);

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    //dataStream << _currentItem->rectangle().center().x() << _currentItem->rectangle().center().y();
    dataStream << _currentItem->rectangle().x() << _currentItem->rectangle().y();

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);
    drag->setMimeData(mimeData);

    //  Size image based on current zoom and screen DPI.
    const auto curSize = (screen_block - _margin.left() * grid_to_px * 2) * _currentZoom;
    auto dragPix = _currentItem->image()->scaledToHeight(curSize, Qt::SmoothTransformation);
    drag->setPixmap(dragPix);

    QPointF offset{curSize / 2, curSize / 2};
    drag->setHotSpot(offset.toPoint());
    setCursor(Qt::OpenHandCursor);

    //  If this function is not called, the drag will not start
    drag->exec();
}
