#include "graphiccanvas.hpp"
#include <QAction>      //  For context menu
#include <QApplication> //  For startDragDistance()
#include <QCursor>
#include <QDrag>
#include <QMenu>
#include <QPainter>
#include <QSvgRenderer>
#include <Qt> //  Keyboard constants
#include "schematic/blueprintlibrary.hpp"
#include "schematic/circuitproject.hpp"

GraphicCanvas::GraphicCanvas(QQuickItem *parent) : QQuickPaintedItem(parent) {
  _project = std::make_shared<CircuitProject>();
  // Magic constant 8.
  _project->add_builtin_blueprints((i16)8);
  _project->add_test_data((i16)8);

  _mipmaps = std::make_shared<MipmapStore>(_project);

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
}

void GraphicCanvas::setOriginX(float x) {
  _top_left.set_x(x / grid_to_px / _currentZoom);
  emit originChanged();
  update();
}

void GraphicCanvas::setOriginY(float y) {
  _top_left.set_y(y / grid_to_px / _currentZoom);
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

void GraphicCanvas::setBlueprint(u32 bp) {
  schematic::BlueprintID id{bp};
  if (id == _selectedBlueprint) return;
  else if (id == schematic::BlueprintID{}) return;
  _selectedBlueprint = id;
  //  Changing template only affects current item to stamp down
  //  Does not require a redraw
  emit blueprintChanged();
}

void GraphicCanvas::setCurrentLine(LineProperties *item) {
  if (item != _currentLine) {
    _currentLine = item;
    update();
    emit currentItemChanged();
  }
}

void GraphicCanvas::setFilter(const BlueprintLibraryModel::Filter filter) {
  if (filter != _filter) {
    _filter = filter;

    emit filterChanged();
  }
}

schematic::MipmapStoreKey GraphicCanvas::cacheSVG(const QString &source) {
  auto mipmap_source = MipmapSource::from_svg_file(source);
  QSize size(48 * 4 * 3, 34 * 4 * 3);
  auto key_for = _mipmaps->insert(mipmap_source, size, Direction::Right, {});
  return key_for;
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

  auto schematic = _project->schematic();
  //  Diagrams are painted on minor grid axis. Overwrite lines.
  for (auto &it : schematic->components()) {
    //  Skip painting rectangles that are outside the viewport.
    auto comp = it.second;
    if (pepp::core::intersects(grid_viewport, comp->geometry())) paint_one(painter, comp.get());
  }

  for (auto &it : schematic->connections()) {
    const auto src = schematic->pin_geometry(it.src);
    const auto dst = schematic->pin_geometry(it.dst);
    const auto span = pepp::core::hull(dst, src);
    // Skip connections entirely outside viewport.
    if (!pepp::core::intersects(grid_viewport, span)) continue;
    const auto screenTo = grid_to_screen(src.center_approximate());
    const auto screenFrom = grid_to_screen(dst.center_approximate());
    QColor color = _normal;
    painter->setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(screenTo, screenFrom);
  }

  if (_currentDragShadow) {
    const auto screen_rect = grid_to_screen(_currentDragShadow->drop_location);
    const QColor bg = _currentDragShadow->has_hit ? QColorConstants::Svg::red : QColorConstants::Svg::green;
    painter->setBrush(bg);
    painter->drawRect(screen_rect);
  }
}

void GraphicCanvas::paint_one(QPainter *painter, Component *comp) {
  // Convert our absolute grid coordinates to screen coordinates.
  // Grid is inset so that selection box appears inside current cell
  auto screen_rect = grid_to_screen(comp->geometry());
  // std::cerr << "Rect Geometry: " << comp->geometry() << std::endl;
  auto props = static_cast<BaseProperties *>(comp->properties.get());
  //  Check state, and set outline if selected
  if (props && props->selected()) {
    painter->setPen(QPen(_highlight, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawRect(screen_rect);
  }

  //  If image is null, then it's properties were reset, update image
  const auto mipmap_key = getImage(comp);

  //  Paint diagram
  // Get mipmaps for the current image.
  const auto mip = _mipmaps->mipmap(mipmap_key);
  const auto best = mip->best_for(screen_rect.size().toSize(), comp->direction());
  painter->drawPixmap(screen_rect.toRect(), *best);

  //  Paint input pins
  painter->setPen(QPen(QColorConstants::Svg::aqua, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  for (const auto &pin : comp->input_pins()) {
    // std::cerr << "ipin geometry: " << pin.geometry << std::endl;
    painter->drawEllipse(grid_to_screen(pin.geometry));
  }

  //  Paint output pins
  painter->setPen(QPen(QColorConstants::Svg::lime, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  for (const auto &pin : comp->output_pins()) {
    // std::cerr << "opin geometry: " << pin.geometry << std::endl;
    painter->drawEllipse(grid_to_screen(pin.geometry));
  }
}

void GraphicCanvas::paint_line(QPainter *painter, Connection con) {}

void GraphicCanvas::addLine(DiagramProperties *from, DiagramProperties *to) {
  //  Calculate maximum line dimensions
  // const auto key = LineProperties::recalculateKey(from, /*to*/ from);

  // TODO: mmcraven, _data.createLineProps(key);
  LineProperties *line = nullptr;

  if (line == nullptr) return;

  //  Set line type
  // line->setType(DiagramType::Line);
  // line->setOutputDiagram(from);
  // line->setInputDiagram(to);
}

std::optional<schematic::ComponentID> GraphicCanvas::place_component(schematic::BlueprintID id,
                                                                     schematic::Point location, Direction dir) {
  auto schematic = _project->schematic();
  auto blueprint = _project->library()->get_blueprint(id);
  auto maybe_id = schematic->place_component(blueprint, location, dir);
  if (maybe_id) {
    auto comp = schematic->component(*maybe_id);
    ensureProperties(comp.get());
    return maybe_id;
  }
  return std::nullopt;
}

void GraphicCanvas::cacheBoundingBox() {
  const auto logicRect = _project->schematic()->bounding_box();
  const auto totalWidth = logicRect.right() * minor_block_size;
  const auto totalHeight = logicRect.bottom() * minor_block_size;
  PeppRect gridRect = PeppRect::from_point_size(0, 0, totalWidth, totalHeight);
  //  See if dimensions changed
  if (_dimensions != gridRect) {
    _dimensions = pepp::core::hull(_dimensions, gridRect);
    emit boundsChanged();
  }
}

schematic::MipmapStoreKey GraphicCanvas::getImage(Component *comp) {
  auto as_builtin = dynamic_cast<const BuiltinBlueprint *>(comp->blueprint());
  if (as_builtin != nullptr) {
    const auto img = comp->blueprint()->image;
    const auto fname = _project->find_file(img);
    if (!fname) throw std::logic_error("Image key does not correspond to a tracked file");
    const auto maybe_mipmap = _mipmaps->find(**fname);
    if (!maybe_mipmap) return cacheSVG(QString::fromStdString(**fname));
    return *maybe_mipmap;
  }
  throw std::logic_error("Unimplemented image type");
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
  if (std::holds_alternative<Component *>(_selected)) {
    auto comp = std::get<Component *>(_selected);
    if (_project->schematic()->rotate_component(comp->id(), clockwise(comp->direction()))) update();
  }
}

void GraphicCanvas::rotateCounterClockwise() {
  if (std::holds_alternative<Component *>(_selected)) {
    auto comp = std::get<Component *>(_selected);
    if (_project->schematic()->rotate_component(comp->id(), counter_clockwise(comp->direction()))) update();
  }
}

//  Mouse events - Comment out unused events for now

void GraphicCanvas::mouseMoveEvent(QMouseEvent *event) {
  // qDebug() << "MouseMove" << event;
  if (!(event->buttons() & Qt::LeftButton)) return;
  if ((event->pos() - _dragStartPosition).manhattanLength() < QApplication::startDragDistance()) return;

  //  Mouse location in grid coordinates to
  //  to determine rectangle hit.
  const auto point = screen_to_grid(event->position());

  //  See if existing item was clicked, if so begi
  if (_project->schematic()->component_at(point)) {
    setSelectedDiagram(point);
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

  //  Check if context menu
  if (event->button() == Qt::RightButton) {
    if (_filter != BlueprintLibraryModel::Line) contextMenuEvent(event);
    return;
  }

  //  Mouse location in grid coordinates to
  //  to determine rectangle hit.
  const auto point = screen_to_grid(event->position());

  //  Prioritize diagram hits over line hits
  const auto areDiagrams = setSelectedDiagram(point);
  if (areDiagrams || _filter == BlueprintLibraryModel::Diagram) {
    //  See if existing item was clicked
    if (areDiagrams) {
      if (_filter == BlueprintLibraryModel::Line) {
        //  If filter is a line, then we are adding line
        // TODO: mmcraven lineLeftClickEvent(event, _currentDiagram);
      } else {
        //  If diagram, we are handling a drag event
        _dragStartPosition = event->position();
      }
    } else diagramLeftClickEvent(event, point);

    return;
  }

  //  User did not pick diagram, see if line is selected
  setSelectedLine(point);
}

void GraphicCanvas::diagramLeftClickEvent(QMouseEvent *event, const PeppPt &point) {
  //  No template is selected, just return
  if (_selectedBlueprint == schematic::BlueprintID{}) {
    event->setAccepted(false);
    return;
  }

  // TODO: another copy of that magic 8 constant.
  const auto index = (i16)8 * grid_to_index(point);
  //  If we get here, we have a new item. Insert into canvas
  //  Use coordinate as center point
  auto id = place_component(_selectedBlueprint, index, Direction::Right);
  //  If no data is returned, the column is invalid. Assume parent will handle
  event->setAccepted(id.has_value());
}

void GraphicCanvas::lineLeftClickEvent(QMouseEvent *event, DiagramProperties *current) {

  if (current == nullptr) {
    event->setAccepted(false);
  }

  if (_firstPoint == nullptr) {
    //  User clicked on start diagram
    _firstPoint = current;
  } else {
    //  User clicked on endpoint. Create line
    addLine(_firstPoint, current);

    //  Reset first point
    _firstPoint = nullptr;

    //  Refresh screen to show line
    update();
  }
  event->setAccepted(true);
}

bool GraphicCanvas::setSelectedDiagram(const PeppPt &point) {
  auto schematic = _project->schematic();

  //  If selecting diagram, then unselect all lines
  unselectLines();

  _selected = std::monostate{};
  //  See if existing item was clicked and clear selection
  for (auto &it : schematic->components()) {
    const auto [id, comp] = it;
    auto props = static_cast<BaseProperties *>(comp->properties.get());
    // Don't break, because we want to ensure all other items are unselected.
    if (pepp::core::contains(comp->geometry(), point)) {
      _selected = comp.get();
      ensureProperties(comp.get());
      static_cast<BaseProperties *>(comp->properties.get())->setSelected(true);
    } else {
      if (props != nullptr && props->selected()) props->setSelected(false);
    }
  }

  //  Repaint
  update();

  return std::holds_alternative<Component *>(_selected);
}

bool GraphicCanvas::setSelectedLine(const PeppPt &point) {
  auto schematic = _project->schematic();
  bool found{false};

  //  Line was selected. Clear diagram selection
  unselectDiagrams();

  //  See if existing item was clicked and clear selection
  for (auto &props : schematic->connections()) {
    // TODO:
    // Skip painting rectangles that are outside the viewport.
    /*if (!pepp::core::contains(props->gridRectangle(), point)) {
      if (props->selected()) {
        //  Item was previously selected, clear old highlight
        //  Bypass model
        props->setSelected(false);

        //  Update unselected rectangle
        setCurrentLine(nullptr);
      }
      continue;
    }*/

    // TODO: Bypass model
    // props->setSelected(true);
    // setCurrentLine(props.get());

    //  Signal update to QML controls
    // TODO: mmcraven, modify selection for propert viewer.
    found = true;
  }
  //  Repaint
  update();

  return found;
}

//  Used to clear all diagram selections
void GraphicCanvas::unselectDiagrams() {
  for (auto &it : _project->schematic()->components()) {
    const auto [id, comp] = it;
    auto props = static_cast<BaseProperties *>(comp->properties.get());
    if (props != nullptr && props->selected()) props->setSelected(false);
  }
  if (std::holds_alternative<Component *>(_selected)) {
    _selected = std::monostate{};
  };
}

//  Used to clear all line selections
void GraphicCanvas::unselectLines() {
  for (auto &props : _project->schematic()->connections()) {
    // TODO: mmcraven props->setSelected(false);
  }

  //  Update unselected rectangle
  setCurrentLine(nullptr);
}

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
    cacheBoundingBox();
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
    cacheBoundingBox();
  }

  //  Update screen
  setOriginX(x);
}

void GraphicCanvas::moveComponent(PeppPt oldLocation, PeppPt newLocation, bool enforce_alignment) {
  auto schematic = _project->schematic();
  auto maybe_component_id = schematic->component_at(oldLocation);
  if (maybe_component_id) return moveComponent(*maybe_component_id, newLocation, enforce_alignment);
}

void GraphicCanvas::moveComponent(schematic::ComponentID id, PeppPt newLocation, bool enforce_alignment) {
  auto schematic = _project->schematic();
  const auto comp = schematic->component(id);
  const auto alignedLocation =
      enforce_alignment ? comp->blueprint()->alignmentConstraint.nearest_aligned_point(newLocation) : newLocation;
  if (!schematic->can_move_component(id, alignedLocation)) return;
  schematic->move_component(id, alignedLocation);
  cacheBoundingBox();
  update();
}

void GraphicCanvas::rotateComponent(schematic::ComponentID id) {
  auto schematic = _project->schematic();
  auto comp = schematic->component(id);
  if (!comp) return;
  const auto current_orientation = comp->direction();
  const auto next_orientation = clockwise(current_orientation);
  if (!schematic->can_rotate_component(id, next_orientation)) return;
  else schematic->rotate_component(id, next_orientation);
  cacheBoundingBox();
  update();
}

void GraphicCanvas::dragEnterEvent(QDragEnterEvent *event) {
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
  // Stopped dragging w/o dropping. Must clear drag shadow and update, else we keep rendering the shadow rectangle until
  // the next drag event.
  _currentDragShadow = std::nullopt;
  update();
  event->ignore();
}

void GraphicCanvas::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
    auto oldShadow = _currentDragShadow;
    bool has_hit = hitTest(event->position());
    if (oldShadow != _currentDragShadow) update();
    if (has_hit) return;

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
  qDebug() << "dropEvent";
  if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
    // setCursor(Qt::ArrowCursor);

    if (hitTest(event->position())) {
      qDebug() << "bailing early";
      _currentDragShadow = std::nullopt;
      update();
      //  There is a hit on an existing item, abort move
      return;
    }

    //  Get original data
    QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);

    schematic::ComponentID id;
    dataStream >> id.value;

    const auto point = screen_to_grid(event->position());
    qDebug() << _dragStartPosition << "dropEvent: " << event->position() << "grid point: " << point.x() << ","
             << point.y();
    const auto comp = _project->schematic()->component(id);
    // IDK where the magic constant "8" comes from, but removing it causes drag+drop to fail.
    const auto dropLocation = _currentDragShadow->drop_location.top_left();
    moveComponent(id, dropLocation);
    _currentDragShadow = std::nullopt;

    //  Remap paint grid after move
    cacheBoundingBox();
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
  setSelectedDiagram(screen_to_grid(point));
  if (!std::holds_alternative<Component *>(_selected)) return;
  auto comp = std::get<Component *>(_selected);

  //  Setup drag operation
  _dragStartPosition = point;
  QDrag *drag = new QDrag(this);

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);

  //  Save old data to stream
  dataStream << comp->id().value;

  QMimeData *mimeData = new QMimeData;
  mimeData->setData("application/x-dnditemdata", itemData);
  drag->setMimeData(mimeData);

  //  Size image based on current zoom and screen DPI. Margin is in grid coordinates, and
  //  there are 2 equal margins.
  const auto curSize = (screen_block - (_margin * grid_to_px * 2)) * _currentZoom;

  QPixmap dragPix;

  const auto key = getImage(comp);
  if (!_mipmaps->contains(key)) {
    qWarning() << "No mipmaps found for diagram :" << comp->id().value;
    return;
  }
  const auto geom = comp->geometry();
  if (geom.width() > geom.height()) {
    dragPix = _mipmaps->mipmap(key)
                  ->best_for(QSize(curSize, curSize), comp->direction())
                  ->scaledToWidth(curSize, Qt::SmoothTransformation);
  } else {
    dragPix = _mipmaps->mipmap(key)
                  ->best_for(QSize(curSize, curSize), comp->direction())
                  ->scaledToHeight(curSize, Qt::SmoothTransformation);
  }

  drag->setPixmap(dragPix);

  //  Use center point for hit detection
  QPointF offset{curSize / 2, curSize / 2};
  drag->setHotSpot(offset.toPoint());

  //  If this function is not called, the drag will not start
  drag->exec();
}

bool GraphicCanvas::hitTest(QPointF newPoint) const {
  //  Mouse location in grid coordinates to
  //  to determine rectangle hit.
  const auto point = screen_to_grid(newPoint);
  const auto schematic = _project->schematic();

  //  This function can be called dozens of times per second when dragging an item.
  //  The canMoveData function can be expensive if called excessively. Cache
  //  prior result, and only call canMoveData when we have moved to a new grid area.
  static bool lastResult{false};
  static PeppPt lastPt{-1, -1};
  /*qDebug() << "hitTest: " << newPoint << "lastPt: " << grid_to_screen(lastPt) << "lastResult: " << lastResult
           << "has selected" << std::holds_alternative<Component *>(_selected);*/
  if (lastPt != point && std::holds_alternative<Component *>(_selected)) {
    const auto comp = std::get<Component *>(_selected);
    const auto geom = comp->geometry();
    // Compute the size of the item being dragged in the screen space
    const auto tl = grid_to_screen(geom.top_left()), br = grid_to_screen(geom.bottom_right());
    const auto screenRect = QRectF(tl, br);
    // Shift newPoint by 1/2 the size of screen-space rectangle so that hit test is based on the center.
    // Shift MUST be done in screen space, else we get weird quantization bugs of algnment constraints.
    const auto shifted = newPoint - QPointF(screenRect.width() / 2, screenRect.height() / 2);
    const auto unaligned = screen_to_grid(shifted);
    auto aligned = comp->blueprint()->alignmentConstraint.nearest_aligned_point(unaligned);
    lastPt = point;
    lastResult = schematic->can_move_component(comp->id(), aligned);
    _currentDragShadow =
        DragRect{.has_hit = !lastResult, .drop_location = schematic::Rectangle(aligned, comp->geometry().size())};
  }
  //  Can move is True if there is no hit. Flip to indicate if hit or not
  return !lastResult;
}

bool GraphicCanvas::hasSelectedComponent() { return std::holds_alternative<Component *>(_selected); }

void GraphicCanvas::ensureProperties(Component *comp) {
  if (comp->properties == nullptr) {
    comp->properties = std::make_unique<DiagramProperties>(nullptr);
  }
}

bool GraphicCanvas::keyPress(const int key, const int modifier) {
  const bool alt = modifier & Qt::AltModifier;
  const bool shf = modifier & Qt::ShiftModifier;
  const bool ctr = modifier & Qt::ControlModifier;
  // TODO: mmcraven, reenable active/selection

  switch (key) {
  case Qt::Key_Delete:
    if (std::holds_alternative<Component *>(_selected)) {
      auto comp = std::get<Component *>(_selected);
      _selected = std::monostate{};
      _project->schematic()->remove_component(comp->id());
      // TODO: notify QML
      update(); // Clear current item.
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

    if (std::holds_alternative<Component *>(_selected) && !ctr) {
      auto comp = std::get<Component *>(_selected);
      //  We are moving item
      auto start = comp->geometry().top_left();
      auto dest = start.with_x(start.x() - 1);
      dest = comp->blueprint()->alignmentConstraint.nearest_aligned_point_left(dest);
      moveComponent(start, dest, true);
    } else setHScroll(-1);
    return true;
  case Qt::Key_Right:
    if (std::holds_alternative<Component *>(_selected) && !ctr) {
      auto comp = std::get<Component *>(_selected);
      //  We are moving item
      auto start = comp->geometry().top_left();
      auto dest = start.with_x(start.x() + 1);
      dest = comp->blueprint()->alignmentConstraint.nearest_aligned_point_right(dest);
      moveComponent(start, dest);
    } else setHScroll(1);
    return true;
  case Qt::Key_Up:
    if (std::holds_alternative<Component *>(_selected) && !ctr) {
      auto comp = std::get<Component *>(_selected);
      //  We are moving item
      auto start = comp->geometry().top_left();
      auto dest = start.with_y(start.y() - 1);
      dest = comp->blueprint()->alignmentConstraint.nearest_aligned_point_up(dest);
      moveComponent(start, dest);
    } else setVScroll(-1);
    return true;
  case Qt::Key_Down:
    if (std::holds_alternative<Component *>(_selected) && !ctr) {
      auto comp = std::get<Component *>(_selected);
      //  We are moving item
      auto start = comp->geometry().top_left();
      auto dest = start.with_y(start.y() + 1);
      dest = comp->blueprint()->alignmentConstraint.nearest_aligned_point_down(dest);
      moveComponent(start, dest);
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
