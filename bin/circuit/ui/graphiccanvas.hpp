#pragma once

#include <QPixmap>
#include <QQuickPaintedItem>

#include "blueprintlibrarymodel.hpp"
#include "diagramproperty.hpp"
#include "schematic/circuitschematic.hpp"

#include "core/math/geom/rectangle.hpp"
#include "pixmaps/mipmapstore.hpp"

using PeppRect = pepp::core::Rectangle<i16>;
using PeppSize = pepp::core::Size<i16>;
using PeppPt = pepp::core::Point<i16>;

class DiagramDataModel;

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class GraphicCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(GraphicCanvas)

  // Sizes in "screen" coordinates
  Q_PROPERTY(float contentWidth READ contentWidth NOTIFY boundsChanged FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight NOTIFY boundsChanged FINAL)

  // In "screen" coordinates (e.g., pixels according to our containing Flickable)
  Q_PROPERTY(float originX READ originX WRITE setOriginX NOTIFY originChanged FINAL)
  Q_PROPERTY(float originY READ originY WRITE setOriginY NOTIFY originChanged FINAL)

  // Used to shrink canvas to fit in scroll bars
  Q_PROPERTY(float xScrollbar READ xScrollbar WRITE setXScrollbar NOTIFY boundsChanged FINAL)
  Q_PROPERTY(float yScrollbar READ yScrollbar WRITE setYScrollbar NOTIFY boundsChanged FINAL)

  //  Set and access datamodel and template
  Q_PROPERTY(CircuitProject *project READ project NOTIFY projectChanged FINAL)
  Q_PROPERTY(u32 blueprint READ blueprint WRITE setBlueprint NOTIFY blueprintChanged FINAL)
  Q_PROPERTY(u32 componentId READ componentId NOTIFY componentChanged FINAL)
  Q_PROPERTY(BlueprintLibraryModel::Filter filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)

public:
  GraphicCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;

  //  Sets currently selected diagram/line
  Q_INVOKABLE bool hasSelectedComponent() const;
  Q_INVOKABLE void rotateClockwise();
  Q_INVOKABLE void rotateCounterClockwise();

  //  Handle keypress events from QML
  Q_INVOKABLE bool keyPress(const int key, const int modifier);

  // Max bounds based on contained rectangles.
  float contentWidth() const { return _dimensions.width() * grid_to_px * _currentZoom; }
  float contentHeight() const { return _dimensions.height() * grid_to_px * _currentZoom; }
  void cacheBoundingBox();

  // The top-left corner, as measured in "screen" coordinates
  float originX() const { return _top_left.x() * grid_to_px * _currentZoom; }
  float originY() const { return _top_left.y() * grid_to_px * _currentZoom; }

  // Compute grid coordinates from screen coordinates
  void setOriginX(float x);
  void setOriginY(float y);

  // The top-left corner, as measured in "screen" coordinates
  float xScrollbar() const { return _scrollbarWidth.right() * grid_to_px; }
  float yScrollbar() const { return _scrollbarWidth.bottom() * grid_to_px; }

  // Compute grid coordinates from screen coordinates
  void setXScrollbar(float x);
  void setYScrollbar(float y);

  u32 blueprint() const { return _selectedBlueprint.value; }
  void setBlueprint(u32 bp);

  Component *component() const;
  u32 componentId() const;

  LineProperties *currentLine() const { return _currentLine; }
  void setCurrentLine(LineProperties *item);

  auto filter() const { return _filter; }
  void setFilter(const BlueprintLibraryModel::Filter filter);

protected:
  //  Mouse events
  // void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  // void mouseReleaseEvent(QMouseEvent *event) override;
  /*void mouseUngrabEvent() override;

  void hoverEnterEvent(QHoverEvent *event) override;
  void hoverLeaveEvent(QHoverEvent *event) override;
  void hoverMoveEvent(QHoverEvent *event) override;*/

  void wheelEvent(QWheelEvent *event) override;

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

signals:
  void boundsChanged();
  void originChanged();
  void currentItemChanged();
  void filterChanged();
  void projectChanged();
  void blueprintChanged();
  void componentChanged();

private:
  //  Render and cache images for painting
  schematic::MipmapStoreKey cacheSVG(const QString &source);
  schematic::MipmapStoreKey getImage(Component *comp);

  //  Custom mouse event handlers
  void contextMenuEvent(QMouseEvent *event);
  void diagramLeftClickEvent(QMouseEvent *event, const PeppPt &point);
  void lineLeftClickEvent(QMouseEvent *event, DiagramProperties *current);

  // Helepr for painting a single rect that has already "passed" the clipping test.
  void paint_one(QPainter *painter, Component *comp);
  void paint_line(QPainter *painter, Connection con);
  QRectF grid_to_screen(const PeppRect &rect) const;
  PeppRect screen_to_grid(QRectF rect) const;
  PeppPt screen_to_grid(QPointF point) const;
  const PeppPt grid_to_index(const PeppPt &point) const;
  QPointF grid_to_screen(const PeppPt &pt) const;

  //  Functions for key and mouse events
  void setZoom(qint8 change);
  void setVScroll(qint8 change);
  void setHScroll(qint8 change);
  void moveComponent(PeppPt oldLocation, PeppPt newLocation, bool enforce_alignment = true);
  void moveComponent(schematic::ComponentID comp, PeppPt newLocation, bool enforce_alignment = true);
  void rotateComponent(schematic::ComponentID comp);
  bool hitTest(QPointF newPoint) const;

  inline CircuitProject *project() const { return _project.get(); }
  void ensureProperties(Component *comp);
  bool setSelectedDiagram(const PeppPt &point);
  bool setSelectedLine(const PeppPt &point);
  void unselectDiagrams();
  void unselectLines();


  //  Render and cache background lines
  void cacheBackground();

  //  Add diagram, and center in cell
  void addLine(DiagramProperties *from, DiagramProperties *to);
  std::optional<schematic::ComponentID> place_component(schematic::BlueprintID, schematic::Point location,
                                                        Direction dir);

  //  Respond to data changes in model
  void updateCell(const QModelIndex &from, const QModelIndex &to);

  //  Drag drop functions
  void startDrag(const QPoint point);

  // Magic constant to convert from grid coordinates to screen coordinates
  // Zoom variables
  const qreal grid_to_px = 4.0;
  const qreal _minScale = 0.5;
  const qreal _maxScale = 3.0;
  qreal _currentZoom = 1.0;

  //  Grid dimensions (logical size, screen size is this times grid_to_px
  const i16 lines_per_minor = 4;
  const i16 minor_per_major = 4;
  const i16 line_block_size = 2;
  static constexpr i16 minor_block_size = 8;
  const i16 major_block_size = minor_block_size * minor_per_major;
  const float screen_block = major_block_size * grid_to_px;
  const i16 _margin = 4;

  std::shared_ptr<CircuitProject> _project = nullptr;
  //  Cached images
  std::shared_ptr<MipmapStore> _mipmaps = nullptr;

  // Top-left corner of the viewport in grid coordinates
  PeppPt _top_left;
  PeppRect _dimensions;

  //  Background is saved in screen coordinates since there is no hit testing
  QPixmap _background{major_block_size * 8, major_block_size * 8};

  //  Margins are always in screen coordinates since they do not
  //  interact with the drawing model. They only impact screen clipping
  QMarginsF _scrollbarWidth{0, 0, 0, 0};

  //  Make fixed color for now
  QColor _highlight = QColorConstants::Svg::cornflowerblue;
  QColor _normal = QColorConstants::Svg::black;

  // Selection information
  std::variant<std::monostate, Component *> _selected = std::monostate{};
  schematic::BlueprintID _selectedBlueprint{};

  //  Drag start
  QPointF _dragStartPosition{-1, -1};
  struct DragRect {
    bool has_hit = false;
    schematic::Rectangle drop_location;
    bool operator==(const DragRect &other) const = default;
  };
  // Hold the "drop" coordinate in logical coordinates and if the drop is expected to be valid.
  mutable std::optional<DragRect> _currentDragShadow = std::nullopt;

  //  Line information
  DiagramProperties *_firstPoint = nullptr;
  LineProperties *_currentLine = nullptr;

  //  Data model
  BlueprintLibraryModel::Filter _filter = BlueprintLibraryModel::None;
};
