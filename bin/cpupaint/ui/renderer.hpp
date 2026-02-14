#pragma once
#include <QPixmap>
#include <QQuickPaintedItem>
#include "shapes_one.hpp"

struct TextRectItem {
  QRectF geom;
  QString text;
  QColor color{0, 0, 0, 255};
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

class QMLOverlay : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(QMLOverlay)
  Q_PROPERTY(QRect location READ location CONSTANT FINAL)
public:
  QMLOverlay(QRect location, QObject *parent = nullptr);
  QRect location() const { return _location; }

private:
  QRect _location;
};

// "screen" coordinates are pixels, in a range specified by our containing Flickable.
// "grid" coordinates are integer values. Currently, 1 grid unit = 4 screen pixels, but this should
// be programmable to enable zoom.
class CursedCPUCanvas : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(CursedCPUCanvas)
  // Sizes in "screen" coordinates
  Q_PROPERTY(float contentWidth READ contentWidth CONSTANT FINAL)
  Q_PROPERTY(float contentHeight READ contentHeight CONSTANT FINAL)
  Q_PROPERTY(QList<QMLOverlay *> overlays READ overlays CONSTANT FINAL)
public:
  CursedCPUCanvas(QQuickItem *parent = nullptr);
  ~CursedCPUCanvas() noexcept override;
  void paint(QPainter *painter) override;

  float contentWidth() const { return 900; }
  float contentHeight() const { return 1050; }
  QList<QMLOverlay *> overlays() const { return _overlays; }

private:
  std::array<QPixmap, 5> _arrows;
  std::vector<Item> _geom;
  QList<QMLOverlay *> _overlays;
  friend class PaintDispatch;
};
