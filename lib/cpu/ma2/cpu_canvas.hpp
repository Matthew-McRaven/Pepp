#pragma once
#include <QPixmap>
#include <QQuickPaintedItem>
#include "qml_overlays.hpp"
#include "shapes_one.hpp"

namespace pepp {
struct TextRectItem {
  QRectF geom;
  QString text;
  QColor color{0, 0, 0, 255};
  Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignVCenter;
};

struct RectItem {
  QRect geom;
  QColor bg{0, 0, 0, 255}, fg{0, 0, 0, 255};
  bool enabled = true;
};

struct PolygonItem {
  QPolygon geom;
  QColor bg{0, 0, 0, 255}, fg{0, 0, 0, 255};
  bool enabled = true;
};

struct LineItem {
  QLine geom;
  QColor color{0, 0, 0, 255};
  bool enabled = true;
};

struct ArrowItem {
  Arrow geom;
  QColor color{0, 0, 0, 255};
  bool enabled = true;
};
using Item = std::variant<LineItem, ArrowItem, RectItem, PolygonItem, TextRectItem>;

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class PaintedCPUCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(PaintedCPUCanvas)
  // Sizes in "screen" coordinates
  Q_PROPERTY(float contentWidth READ contentWidth CONSTANT FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight CONSTANT FINAL)
  // Do not try to forward-declare QMLOverlay, else reading the Q_PROPERTY will just return an empty list without an
  // error.
  Q_PROPERTY(QList<QMLOverlay *> overlays READ overlays CONSTANT FINAL)
public:
  PaintedCPUCanvas(QQuickItem *parent = nullptr);
  ~PaintedCPUCanvas() noexcept override;
  void paint(QPainter *painter) override;

  float contentWidth() const { return 900; }
  float contentHeight() const { return 1050; }
  QList<QMLOverlay *> overlays() const { return _overlays; }

private:
  std::array<QPixmap, 5> _arrows;
  std::vector<Item> _geom;
  QList<QMLOverlay *> _overlays;
  friend struct PaintDispatch;
};
} // namespace pepp
