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
static const int OVERLAY_NONE = 0;
static const int OVERLAY_CLOCK = 1;
static const int OVERLAY_TRISTATE = 2;
class QMLOverlay : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(QMLOverlay)
  Q_PROPERTY(QRect location READ location CONSTANT)
  Q_PROPERTY(int type READ type CONSTANT)
public:
  QMLOverlay(QRect location, QObject *parent = nullptr);
  QRect location() const { return _location; }
  virtual int type();

private:
  QRect _location;
};

class ClockOverlay : public QMLOverlay {
  Q_OBJECT
  QML_NAMED_ELEMENT(ClockOverlay)
  Q_PROPERTY(QRect location READ location CONSTANT)
  Q_PROPERTY(int type READ type CONSTANT)
  Q_PROPERTY(QString label READ label CONSTANT FINAL)
  Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)
public:
  ClockOverlay(QRect location, QString label, QObject *parent = nullptr);
  int type() override;
  QString label() const;
  bool value() const;
  void setValue(bool value);
signals:
  void valueChanged();

private:
  QString _label;
  bool _value;
};

class TristateOverlay : public QMLOverlay {
  Q_OBJECT
  QML_NAMED_ELEMENT(TristateOverlay)
  Q_PROPERTY(QRect location READ location CONSTANT)
  Q_PROPERTY(int type READ type CONSTANT)
  Q_PROPERTY(QString label READ label CONSTANT FINAL)
  // 0..N are "valid" values. Any negative value should be considered disabled.
  Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(int maxValue READ max_value CONSTANT FINAL)
public:
  TristateOverlay(QRect location, QString label, int max_value, QObject *parent = nullptr);
  int type() override;
  QString label() const;
  int value() const;
  void setValue(int value);
  int max_value() const;
signals:
  void valueChanged();

private:
  int _max_value, _value;
  QString _label;
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
