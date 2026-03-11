#pragma once

#include <QPixmap>
#include <QQuickPaintedItem>

#include "diagramdatamodel.hpp"
#include "diagramlistmodel.hpp"
#include "diagramproperty.hpp"

#include "core/math/geom/rectangle.hpp"

/*  Rectangle questions
 *  1. To search for an item, I need to know the width/height. Can we lookup
 *  based on just the top left corner? If I do not pass width/height, item is not found.
 *  In future, we will have diagrams of various sizes. I will not always know the width/height
 *  of the item I'm looking up.
 */

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
  Q_PROPERTY(DiagramDataModel *model READ model WRITE setModel NOTIFY boundsChanged FINAL)
  Q_PROPERTY(DiagramTemplate *template READ stamp WRITE setStamp NOTIFY stampChanged FINAL)
  Q_PROPERTY(DiagramProperties *currentItem READ currentItem NOTIFY currentItemChanged FINAL)
  Q_PROPERTY(FilterDiagramListModel::Filter filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)

public:
  GraphicCanvas(QQuickItem *parent = nullptr);
  void paint(QPainter *painter) override;

  //  Context menu (right click)
  Q_INVOKABLE void rotateClockwise();
  Q_INVOKABLE void rotateCounterClockwise();

  //  Handle keypress events from QML
  Q_INVOKABLE bool keyPress(const int key, const int modifier);

  // Max bounds based on contained rectangles.
  float contentWidth() const { return _dimensions.width() * grid_to_px * _currentZoom; }
  float contentHeight() const { return _dimensions.height() * grid_to_px * _currentZoom; }
  void setBoundingBox();

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

  DiagramDataModel *model() const { return _model; }
  void setModel(DiagramDataModel *model);

  DiagramTemplate *stamp() const { return _template; }
  void setStamp(DiagramTemplate *stamp);

  DiagramProperties *currentItem() const { return _currentItem; }
  void setCurrentItem(DiagramProperties *item);

  auto filter() const { return _filter; }
  void setFilter(const FilterDiagramListModel::Filter filter);

protected:
  //  Mouse events
  // void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
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
  void modelChanged();
  void originChanged();
  void stampChanged();
  void currentItemChanged();
  void filterChanged();

private:
  void getImage(DiagramProperties &props);

  //  Custom mouse event handlers
  void contextMenuEvent(QMouseEvent *event);
  void mouseLeftClickEvent(QMouseEvent *event, const PeppPt &index);

  // Helepr for painting a single rect that has already "passed" the clipping test.
  void paint_one(QPainter *painter, const PeppRect &rect, DiagramProperties &props);
  void paint_line(QPainter *painter, const DiagramProperties &props);
  QRectF grid_to_screen(const PeppRect &rect);
  PeppRect screen_to_grid(QRectF rect);
  PeppPt screen_to_grid(QPointF point);
  const PeppPt grid_to_index(const PeppPt &point) const;

  //  Functions for key and mouse events
  void setZoom(qint8 change);
  void setVScroll(qint8 change);
  void setHScroll(qint8 change);
  void moveDiagram(PeppPt oldLocation, PeppPt newLocation);

  //  Sets currently selected diagram
  bool setSelected(const PeppPt &point);

  //  Render and cache images for painting
  void cacheImages(const QString &source);

  //  Render and cache background lines
  void cacheBackground();

  //  Insert test data
  void updateData();

  //  Add diagram, and center in cell
  DiagramProperties *addDiagram(const i16 row, const i16 col);
  void setGrid(DiagramProperties *data, const i16 row, const i16 col);

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
  const i16 minor_per_major = 4;
  const i16 minor_block_size = 8;
  const i16 major_block_size = minor_block_size * minor_per_major;
  const float screen_block = major_block_size * grid_to_px;
  const i16 _margin = 4;

  //  Cached images
  QList<QPixmap> _svgs;
  QList<QPixmap> _svgsBottom;
  QList<QPixmap> _svgsLeft;
  QList<QPixmap> _svgsTop;

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
  QColor _line = QColorConstants::Svg::black;

  //  Drag start
  QPointF _dragStartPosition{-1, -1};

  //  Data model
  DiagramDataModel *_model = nullptr;
  DiagramTemplate *_template = nullptr;
  DiagramProperties *_currentItem = nullptr;
  FilterDiagramListModel::Filter _filter = FilterDiagramListModel::None;
};
