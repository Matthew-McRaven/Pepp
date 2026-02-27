#pragma once
#include <QPixmap>
#include <QQuickPaintedItem>
#include "dataflow.hpp"
#include "qml_overlays.hpp"
#include "settings/constants.hpp"
#include "shapes_one.hpp"

namespace pepp {
struct TextRectItem {
  QRectF geom;
  QString text;
  pepp::settings::PaletteRole role = pepp::settings::PaletteRole::BaseRole;
  Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignVCenter;
};

struct RectItem {
  QRect geom;
  pepp::settings::PaletteRole role = pepp::settings::PaletteRole::BaseRole;
  Connections connection = Connections::None;
  bool enabled = true;
};

struct PolygonItem {
  QPolygon geom;
  pepp::settings::PaletteRole role = pepp::settings::PaletteRole::BaseRole;
  Connections connection = Connections::None;
  bool enabled = true;
};

struct LineItem {
  QLine geom;
  pepp::settings::PaletteRole role = pepp::settings::PaletteRole::BaseRole;
  Connections connection = Connections::None;
  bool enabled = true;
};

struct ArrowItem {
  Arrow geom;
  pepp::settings::PaletteRole role = pepp::settings::PaletteRole::BaseRole;
  Connections connection = Connections::None;
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
  Q_PROPERTY(pepp::ConnectionsHolder *connections READ connections WRITE setConnections NOTIFY connectionsChanged FINAL)
  QML_UNCREATABLE("Please use one of the various subclasses to intialize geometry correctly")
public:
  enum class Which {
    Pep9OneByte = 0,
    Pep9TwoByte = 1,
  };
  PaintedCPUCanvas(Which, QQuickItem *parent = nullptr);
  ~PaintedCPUCanvas() noexcept override;
  void paint(QPainter *painter) override;

  float contentWidth() const { return _w; }
  float contentHeight() const { return _h; }
  QList<QMLOverlay *> overlays() const { return _overlays; }
  ConnectionsHolder *connections() const { return _connections; }
  void setConnections(ConnectionsHolder *c) {
    if (c == _connections) return;
    _connections = c;
    emit connectionsChanged();
  }
signals:
  void connectionsChanged();

private:
  float _h, _w;
  std::array<QPixmap, 5> _arrows;
  std::vector<Item> _geom;
  QList<QMLOverlay *> _overlays;
  ConnectionsHolder *_connections = nullptr;
  friend struct PaintDispatch;
};

class Painted1ByteCanvas : public PaintedCPUCanvas {
  Q_OBJECT
  QML_NAMED_ELEMENT(Painted1ByteCanvas)
public:
  Painted1ByteCanvas(QQuickItem *parent = nullptr);
};

class Painted2ByteCanvas : public PaintedCPUCanvas {
  Q_OBJECT
  QML_NAMED_ELEMENT(Painted2ByteCanvas)
public:
  Painted2ByteCanvas(QQuickItem *parent = nullptr);
};

} // namespace pepp
