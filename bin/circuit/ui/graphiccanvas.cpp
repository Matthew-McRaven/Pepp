#include "graphiccanvas.hpp"
#include <QAction>      //  For context menu
#include <QApplication> //  For startDragDistance()
#include <QCursor>
#include <QDrag>
#include <QMenu>
#include <QPainter>
#include <QSvgRenderer>
#include <Qt> //  Keyboard constants
#include "diagramdatamodel.hpp"

GraphicCanvas::GraphicCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  //  Enable mouse
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true); // Enable hover events if needed

  setAntialiasing(true);

  //  Allow drag and drop.
  //  Disable direct key entry. Handled through QML
  setFlag(QQuickItem::ItemAcceptsDrops, true);
  // setFlag(ItemAcceptsInputMethod, false);

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

void GraphicCanvas::setOriginX(float x) {
  _top_left.setX(x / grid_to_px / _currentZoom);
  emit originChanged();
  update();
}

void GraphicCanvas::setOriginY(float y) {
  _top_left.setY(y / grid_to_px / _currentZoom);
  emit originChanged();
  update();
}

void GraphicCanvas::setXScrollbar(float x) {
  if (std::abs(x - _scrollbarWidth.right()) > .0001) {
    _scrollbarWidth.setRight(x);
    emit boundsChanged();
    update();
  }
}
void GraphicCanvas::setYScrollbar(float y) {
  if (std::abs(y - _scrollbarWidth.bottom()) > .0001) {
    _scrollbarWidth.setBottom(y);
    emit boundsChanged();
    update();
  }
}

void GraphicCanvas::setModel(DiagramDataModel *model) {
  if (model != _model) {
    _model = model;
    emit modelChanged();
    update();
  }
}

void GraphicCanvas::setStamp(DiagramTemplate *stamp) {
  if (stamp != _template) {
    if (stamp == nullptr) {
      _template = nullptr;
    } else {
      //  Is valid stamp
      if (stamp->diagramType() == "Diagram") {
        _template = stamp;
      } else {
        _template = nullptr;
      }
    }
    //  Changing template only affects current item to stamp down
    //  Does not require a redraw
    emit stampChanged();
  }
}

void GraphicCanvas::setCurrentItem(DiagramProperties *item) {
  if (item != _currentItem) {
    _currentItem = item;
    update();
    emit currentItemChanged();
  }
}

void GraphicCanvas::setFilter(const FilterDiagramListModel::Filter filter) {
  if (filter != _filter) {
    _filter = filter;

    emit filterChanged();
  }
}

void GraphicCanvas::updateData() { //  Trigger repaint on data model updates
  //  Model must exist before it can be connected.
  QObject::connect(_model, &DiagramDataModel::dataChanged, this, &GraphicCanvas::updateCell);

  //  Test data-remove below in prod
  static std::array<QString, 6> lookup{"AND Gate", "OR Gate", "Inverter", "NAND Gate", "NOR Gate", "XOR Gate"};
  if (_model == nullptr) return;

  //  Loop to create blocks. Fill Full Grid
  const int rows = 10;
  const int cols = 10;

  DiagramProperties *from = addDiagram(2, 3);
  if (from == nullptr) return;

  //  Add block data
  from->setName(lookup[0]);
  from->setType(DiagramType::ANDGate);
  from->setOrientation(0);
  from->setSelected(false);
  getImage(*from);

  /*MATTHEW START TEST DATA*/

  //  Working example
  /*const DiagramProperties *data1 = _model->dataModel().getDiagramProps(PeppRect::from_point_size(2, 3, 4, 4));
  Q_ASSERT(data1 != nullptr);
  Q_ASSERT(from->id() == data1->id());

  //  Search by id
  const DiagramProperties *data2 = _model->dataModel().getDiagramProps(from->id());
  Q_ASSERT(data2 != nullptr);
  Q_ASSERT(from->id() == data2->id());

  //  Search by point
  const DiagramProperties *data3 = _model->dataModel().getDiagramProps({2, 3});
  Q_ASSERT(data3 != nullptr);
  Q_ASSERT(from->id() == data3->id());
  // MATTHEW END TEST DATA*/

  //  data life time managed by model
  DiagramProperties *to = addDiagram(4, 7);
  if (to == nullptr) return;

  //  Add block data
  to->setName(lookup[2]);
  to->setType(DiagramType::Inverter);
  to->setOrientation(90);
  getImage(*to);

  //  This is a line, but it's not connected
  //  For testing only
  /*addLine(from, to);*/

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

void GraphicCanvas::cacheImages(const QString &source) {
  //  Pre-render SVG files so that paint is not slowed down
  QSvgRenderer renderer(source);
  renderer.setAspectRatioMode(Qt::KeepAspectRatio);

  if (renderer.isValid()) {
    //  SVG dimensions should not matter, but rendering SVG at anything
    //  but a direct multiple of the width creates visual issues.
    int dim = 48 * 3; //_background.width() * grid_to_px;
    // qDebug() << "dim, width, widthMM, logicalDpiX" << dim << _background.width()
    //          << _background.widthMM() << _background.logicalDpiX();
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

void GraphicCanvas::cacheBackground() {
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

void GraphicCanvas::paint(QPainter *painter) {
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
  // qDebug() << "row: " << row << "col:" << col;

  //  Offset first cell if first row or column is cut off
  qreal cX = std::fmod(grid_viewport.left(), major_block_size) * grid_to_px * _currentZoom;
  qreal cY = std::fmod(grid_viewport.top(), major_block_size) * grid_to_px * _currentZoom;

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

  //  Draw lines first. Diagrams will clip lines
  for (auto &prop : _model->dataModel().lines()) {
    auto *line = prop.get();
    //  Skip painting rectangles that are outside the viewport.
    if (pepp::core::intersects(grid_viewport, line->gridRectangle())) paint_line(painter, line);
  }

  //  Diagrams are painted on minor grid axis. Overwrite lines.
  for (auto &prop : _model->dataModel().cells()) {
    //  Skip painting rectangles that are outside the viewport.
    auto *diagram = prop.get();
    if (pepp::core::intersects(grid_viewport, diagram->gridRectangle())) paint_one(painter, diagram);
  }
  // qDebug() << "grid_viewport: " << grid_viewport;
}

void GraphicCanvas::paint_one(QPainter *painter, DiagramProperties *props) {
  // Convert our absolute grid coordinates to screen coordinates.
  // Grid is inset so that selection box appears inside current cell
  auto screen_rect = grid_to_screen(props->gridRectangle()).adjusted(2, 2, -2, -2);
  //  Check state, and set outline if selected
  if (props->selected()) {
    painter->setPen(QPen(_highlight, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawRect(screen_rect);
  }

  //  If image is not null, it can be output
  if (props->image() == nullptr)

    //  If image is null, then it's properties were reset, update image
    getImage(*props);

  //  Print grid is slightly out of alignment with background.
  //  This is a shim to match diagram placement with background.
  screen_rect.adjust(-3, 1, -3, 1); // 0,2,0,0

  painter->drawPixmap(screen_rect.toRect(), *props->image());
}

void GraphicCanvas::paint_line(QPainter *painter, const LineProperties *props) {
  // Convert our absolute grid coordinates to screen coordinates.
  // Grid is inset so that selection box appears inside current cell
  const auto screenTo = grid_to_screen(props->outputPoint());
  const auto screenFrom = grid_to_screen(props->inputPoint());

  //  Check state, and set outline if selected
  painter->setPen(QPen(_line, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter->drawLine(screenTo, screenFrom);
}

DiagramProperties *GraphicCanvas::addDiagram(const i16 row, const i16 col) {
  //  Center point may put diagram off of page, return if either index is negative.
  if (col < 0 || row < 0) return nullptr;

  //  Create index and check for data
  const auto newIndex = _model->index(row, col);

  DiagramProperties *data =
      _model->dataModel().createDiagramProps(PeppRect::from_point_size(row, col, minor_per_major, minor_per_major));
  if (data == nullptr) return nullptr;

  //  Newly added items are always current item
  setCurrentItem(data);

  //  Add block data
  setGrid(data, row, col);

  if (_template != nullptr) {
    data->setName(_template->name());
    data->setType(_template->key());
  }
  getImage(*data);

  //  Set select flag
  _model->setData(newIndex, true, DiagramProperty::Role::Selected);

  return data;
}

void GraphicCanvas::addLine(DiagramProperties *from, DiagramProperties *to) {
  //  Calculate maximum line dimensions
  const auto key = LineProperties::recalculateKey(from, to);

  LineProperties *line = _model->dataModel().createLineProps(key);

  if (line == nullptr) return;

  //  Set line type
  line->setType(DiagramType::Line);
  line->setOutputDiagram(from);
  line->setInputDiagram(to);
}

void GraphicCanvas::setBoundingBox() {
  const auto logicRect = _model->dataModel().boundingRect();
  const auto totalWidth = logicRect.right() * minor_block_size;
  const auto totalHeight = logicRect.bottom() * minor_block_size;
  PeppRect gridRect = PeppRect::from_point_size(0, 0, totalWidth, totalHeight);
  //  See if dimensions changed
  if (_dimensions != gridRect) {
    _dimensions = pepp::core::hull(_dimensions, gridRect);

    emit boundsChanged();
  }
}

void GraphicCanvas::setGrid(DiagramProperties *data, const i16 row, const i16 col) {
  //  Column and row represents center point, not top left
  //  Save in grid coordinates, not screen coordinates
  PeppRect gridRect = PeppRect::from_point_size(minor_block_size * row - major_block_size / 2 + _margin,
                                                minor_block_size * col - major_block_size / 2 + _margin,
                                                major_block_size - _margin * 2, major_block_size - _margin * 2);

  //  Add block data
  data->setGridRectangle(gridRect);

  //  Track dimensions of canvas area. Affects scrollbars
  setBoundingBox();
}

void GraphicCanvas::getImage(DiagramProperties &props) {
  QPixmap *image = nullptr;

  //  If type has not been selected, just return.
  if (props.type() == DiagramType::Invalid) return;

  //  Get cached copy for drawing
  switch (props.orientation()) {
  case 90: image = &_svgsBottom[props.type()]; break;
  case 180: image = &_svgsLeft[props.type()]; break;
  case 270: image = &_svgsTop[props.type()]; break;
  default: image = &_svgs[props.type()];
  }
  props.setImage(image);
}

QRectF GraphicCanvas::grid_to_screen(const PeppRect &rect) const {
  const auto x = (rect.left() - _top_left.x()) * grid_to_px;
  const auto y = (rect.top() - _top_left.y()) * grid_to_px;
  const auto width = rect.width() * grid_to_px;
  const auto height = rect.height() * grid_to_px;

  return {x, y, width, height};
}

QPointF GraphicCanvas::grid_to_screen(const PeppPt &pt) const {
  const auto x = (pt.x() - _top_left.x()) * grid_to_px;
  const auto y = (pt.y() - _top_left.y()) * grid_to_px;

  return {x, y};
}

PeppRect GraphicCanvas::screen_to_grid(QRectF rect) const {
  const auto x = static_cast<i16>((rect.x() / grid_to_px + _top_left.x()) / _currentZoom);
  const auto y = static_cast<i16>((rect.y() / grid_to_px + _top_left.y()) / _currentZoom);
  const auto width = static_cast<i16>(rect.width() / grid_to_px / _currentZoom);
  const auto height = static_cast<i16>(rect.height() / grid_to_px / _currentZoom);

  return PeppRect::from_point_size(x, y, width, height);
}

PeppPt GraphicCanvas::screen_to_grid(QPointF point) const {
  const auto x = static_cast<i16>((point.x() / grid_to_px + _top_left.x()) / _currentZoom);
  const auto y = static_cast<i16>((point.y() / grid_to_px + _top_left.y()) / _currentZoom);

  return {x, y};
}

const PeppPt GraphicCanvas::grid_to_index(const PeppPt &point) const {
  //  Images are stored by row and column.
  //  Due to integer math, items closer to next row or column are still in same column/row.
  //  Calculate rounding difference
  const i16 dx = (point.x() % minor_block_size) > (minor_block_size / 2) ? 1 : 0;
  const i16 dy = (point.y() % minor_block_size) > (minor_block_size / 2) ? 1 : 0;
  const i16 x = static_cast<i16>(point.x() / minor_block_size + dx);
  const i16 y = static_cast<i16>(point.y() / minor_block_size + dy);

  return {x, y};
}

void GraphicCanvas::updateCell(const QModelIndex &from, const QModelIndex &to) {
  //  Delegate updates to QT canvas control
  update();
}

void GraphicCanvas::rotateClockwise() {
  if (_currentItem == nullptr) return;

  _currentItem->setOrientation(_currentItem->orientation() + 90);

  //  Repaint rectangle
  update();
}

void GraphicCanvas::rotateCounterClockwise() {
  if (_currentItem == nullptr) return;

  const int orientation = _currentItem->orientation() == 0 ? 270 : _currentItem->orientation() - 90;
  _currentItem->setOrientation(orientation);

  //  Repaint rectangle
  update();
}

//  Mouse events - Comment out unused events for now
/*void GraphicCanvas::mouseDoubleClickEvent(QMouseEvent *event) {}*/

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event) {
  // qDebug() << "MouseMove" << event;
  if (!(event->buttons() & Qt::LeftButton)) return;
  if ((event->pos() - _dragStartPosition).manhattanLength() < QApplication::startDragDistance()) return;

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

void GraphicCanvas::contextMenuEvent(QMouseEvent *event) {
  //  Context menu must be handled in QML since parent application is QML
  //  based. Always return ignore so that parent QML can handle request.
  event->setAccepted(false);
}

void GraphicCanvas::mousePressEvent(QMouseEvent *event) {
  //  Only handle left and right click
  if (!(event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) return;

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
  // qDebug() << event->pos() << event->position();

  //  See if existing item was clicked
  const auto found = setSelected(point);

  //  Check if context menu
  if (event->button() == Qt::RightButton) {
    contextMenuEvent(event);
  } else if (event->button() == Qt::LeftButton) {
    if (found)
      //  Handle drag event
      _dragStartPosition = event->position();
    else {
      //  Images are stored by row and column.
      //  Due to integer math, items closer to next row or column are still in same column/row.
      //  Calculate rounding difference
      const auto index = grid_to_index(point);

      mouseLeftClickEvent(event, index);
    }
  }
}

void GraphicCanvas::mouseLeftClickEvent(QMouseEvent *event, const PeppPt &index) {
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

bool GraphicCanvas::setSelected(const PeppPt &point) {
  bool found{false};

  //  See if existing item was clicked and clear selection
  for (auto &props : _model->dataModel().cells()) {
    // Skip painting rectangles that are outside the viewport.
    if (!pepp::core::contains(props->gridRectangle(), point)) {
      if (props->selected()) {
        //  Item was previously selected, clear old outline
        //  Set through datamodel so that other controls see change
        const auto index = _model->index(props->key().left(), props->key().top());
        _model->setData(index, false, DiagramProperty::Role::Selected);

        //  Update unselected rectangle
        setCurrentItem(nullptr);
      }
      continue;
    }

    //  Item exists and is selected, update view
    //  Save current item for other actions
    //  Set through view so that other controls see change
    const auto index = _model->index(props->key().left(), props->key().top());
    _model->setData(index, true, DiagramProperty::Role::Selected);

    setCurrentItem(props.get());
    //  Update current rectangle
    // update();

    found = true;

    //  Let QML know that current item has changed.
    // emit currentItemChanged();
  }
  return found;
}

/*void GraphicCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // qDebug() << "MouseRelease" << event;
  // setCursor(Qt::ArrowCursor);
}*/

/*
void GraphicCanvas::mouseUngrabEvent() {}

void GraphicCanvas::hoverEnterEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverLeaveEvent(QHoverEvent *event) {}
void GraphicCanvas::hoverMoveEvent(QHoverEvent *event) {}
*/

void GraphicCanvas::wheelEvent(QWheelEvent *event) {
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
  const qint8 change = angleDelta.y();

  float block = screen_block * _currentZoom;

  if (modifier == Qt::NoModifier) {
    setVScroll(change);
  }
  // Process horizontal scrolling if available
  // Normal h-scrolling uses angleDelta.x() != 0. Current mouse does not
  // Trigger h-scrolling. Use normal scrolling with shift key
  if (modifier == Qt::ShiftModifier) {
    setHScroll(change);
  }

  //  Control + mouse wheel triggeres zoom operations
  if (modifier == Qt::ControlModifier) {
    setZoom(change);

    //  Refresh screen on zoom
    update();
  }

  // Accept the event to stop it from propagating to parent items/widgets.
  // If ignored, a parent item might handle the event (e.g., a surrounding Flickable).
  event->accept();
}

void GraphicCanvas::setZoom(qint8 change) {
  qreal newZoom = 0;
  if (change == 0) return;
  else if (change > 0) newZoom = std::min(_maxScale, _currentZoom + .25);
  else newZoom = std::max(_minScale, _currentZoom - .25);

  //  Apply rounding
  _currentZoom = newZoom;
  qDebug() << "new zoom: " << newZoom;
}

void GraphicCanvas::setVScroll(qint8 change) {
  float y = 0.0;
  const float block = screen_block * _currentZoom;

  if (change > 0) {
    // Perform action for scrolling up
    y = std::max(0.0f, originY() - block);
  } else if (change < 0) {
    // Perform action for scrolling down
    y = std::min(contentHeight(), originY() + block);

    //  Moving item down can increase bounding rectangle
    setBoundingBox();
  }

  //  Update screen
  setOriginY(y);
}

void GraphicCanvas::setHScroll(qint8 change) {
  float x = 0.0;
  const float block = screen_block * _currentZoom;
  if (change < 0) {
    // Perform action for scrolling left
    x = std::max(0.0f, originX() - block);
  } else if (change > 0) {
    // Perform action for scrolling right
    x = std::min(contentWidth(), originX() + block);

    //  Moving item right can increase bounding rectangle
    setBoundingBox();
  }

  //  Update screen
  setOriginX(x);
}

void GraphicCanvas::moveDiagram(PeppPt oldLocation, PeppPt newLocation) {
  //  Update grid coordinates
  if (!_model->dataModel().moveData(oldLocation, newLocation)) return;

  //  Update grid coordinates
  DiagramProperties *data = _model->dataModel().getDiagramProps(newLocation);

  if (data == nullptr) {
    return;
  }

  //  Remap paint grid after move
  setGrid(data, newLocation.x(), newLocation.y());

  update();
}

void GraphicCanvas::dragEnterEvent(QDragEnterEvent *event) {
  // qDebug() << "dragEnterEvent";
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

void GraphicCanvas::dragLeaveEvent(QDragLeaveEvent *event) {
  event->ignore();
  // qDebug() << "dragLeaveEvent";
}

bool GraphicCanvas::hitTest(QPointF newPoint) const {
  //  Mouse location in grid coordinates to
  //  to determine rectangle hit.
  const auto point = screen_to_grid(newPoint);
  const auto newLocation = grid_to_index(point);

  //  Can move is True if there is no hit. Flip to indicate if hit or not
  return !_model->dataModel().canMoveData(_currentItem->id(), newLocation);
}

void GraphicCanvas::dragMoveEvent(QDragMoveEvent *event) {
  // qDebug() << "dragMoveEvent";
  if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
    if (hitTest(event->position())) {
      return;
    }

    if (event->source() == this) {
      event->setDropAction(Qt::MoveAction);
      // setCursor(Qt::OpenHandCursor);
      event->accept();
    } else {
      event->acceptProposedAction();
    }
  } else {
    event->ignore();
  }
}

void GraphicCanvas::dropEvent(QDropEvent *event) {
  // qDebug() << "dropEvent";
  if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
    // setCursor(Qt::ArrowCursor);

    if (hitTest(event->position())) {
      //  There is a hit on an existing item, abort move
      return;
    }

    //  Get original data
    QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);

    i16 oldX, oldY;
    dataStream >> oldX >> oldY;

    const PeppPt oldLocation(oldX, oldY);

    const auto point = screen_to_grid(event->position());
    const auto newLocation = grid_to_index(point);

    moveDiagram(oldLocation, newLocation);

    //  Remap paint grid after move
    setGrid(_currentItem, newLocation.x(), newLocation.y());

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

void GraphicCanvas::startDrag(const QPoint point) {
  //  Temporarily remove item from lookup. During hit detection, item
  //  will return true when pointing to self. Item is reset or saved
  //  in drop event.
  if (_currentItem == nullptr) {
    return;
  }

  //  If error setting cache, just return.
  /*if (!_model->dataModel().cacheData(_currentItem->id())) {
    return;
  }*/

  //  Setup drag operation
  _dragStartPosition = point;
  QDrag *drag = new QDrag(this);

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);

  //  Save old data to stream
  dataStream << _currentItem->key().left() << _currentItem->key().top();

  QMimeData *mimeData = new QMimeData;
  mimeData->setData("application/x-dnditemdata", itemData);
  drag->setMimeData(mimeData);

  //  Size image based on current zoom and screen DPI. Margin is in grid coordinates, and
  //  there are 2 equal margins.
  const auto curSize = (screen_block - (_margin * grid_to_px * 2)) * _currentZoom;
  auto dragPix = _currentItem->image()->scaledToHeight(curSize, Qt::SmoothTransformation);
  drag->setPixmap(dragPix);

  //  Use center point for hit detection
  QPointF offset{curSize / 2, curSize / 2};
  drag->setHotSpot(offset.toPoint());

  // setCursor(Qt::OpenHandCursor);

  //  If this function is not called, the drag will not start
  drag->exec();
}

bool GraphicCanvas::keyPress(const int key, const int modifier) {
  const bool alt = modifier & Qt::AltModifier;
  const bool shf = modifier & Qt::ShiftModifier;
  const bool ctr = modifier & Qt::ControlModifier;

  switch (key) {
  case Qt::Key_Delete:
    if (_currentItem != nullptr) {
      _model->dataModel().clearDiagramData(_currentItem->key());

      //  Clear current item, and notify QML
      setCurrentItem(nullptr);
    }
    return true;

  case Qt::Key_Home:
    setOriginY(0);
    setOriginX(0);
    return true;
    // case Qt::Key_End:*/
    //  Nove item is current. Othewise move background.
    //  Control also means move background
  case Qt::Key_Left:
    if (_currentItem != nullptr && !ctr) {
      //  We are moving item
      PeppPt newLocation = _currentItem->key().top_left();
      //  Do not move beyond left margin
      newLocation.setX(std::max(2, newLocation.x() - 1));
      moveDiagram(_currentItem->key().top_left(), newLocation);
    } else setHScroll(-1);
    return true;
  case Qt::Key_Right:
    if (_currentItem != nullptr && !ctr) {
      //  We are moving item
      PeppPt newLocation = _currentItem->key().top_left();
      newLocation.setX(newLocation.x() + 1);
      moveDiagram(_currentItem->key().top_left(), newLocation);
    } else setHScroll(1);
    return true;
  case Qt::Key_Up:
    if (_currentItem != nullptr && !ctr) {
      //  We are moving item
      PeppPt newLocation = _currentItem->key().top_left();
      //  Do not move beyond top margin
      newLocation.setY(std::max(2, newLocation.y() - 1));
      moveDiagram(_currentItem->key().top_left(), newLocation);
    } else setVScroll(-1);
    return true;
  case Qt::Key_Down:
    if (_currentItem != nullptr && !ctr) {
      //  We are moving item
      PeppPt newLocation = _currentItem->key().top_left();
      newLocation.setY(newLocation.y() + 1);
      moveDiagram(_currentItem->key().top_left(), newLocation);
    } else setVScroll(1);
    return true;
    /*case Qt::Key_PageUp:
    case Qt::Key_PageDown: */
  case Qt::Key_Plus:
    setZoom(1);
    update();
    return true;
  case Qt::Key_Minus:
    setZoom(-1);
    update();
    return true;
  }
  // qDebug() << "Key: " << key << "Modifier:" << modifier;
  return false;
}
