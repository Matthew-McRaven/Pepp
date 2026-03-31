#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE
#include <QObject>
#include <QRect>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "core/math/geom/rectangle.hpp"

#include "diagramtype.hpp"
using PeppRect = pepp::core::Rectangle<i16>;
using PeppSize = pepp::core::Size<i16>;
using PeppPt = pepp::core::Point<i16>;

//  Forward declarations
class DiagramProperties;

struct BaseProperty {
  u32 id = 0;

  //  Gate properties
  DiagramType::Type type = DiagramType::Invalid;
  //  Diagram grid dimensions & placement
  PeppRect key{PeppPt{999, 999}, PeppSize{999, 999}};
  //  Display dimensions & placement
  PeppRect gridRect{};

  bool isSelected = false;
};

struct LineProperty {
  PeppPt input;
  PeppPt output;
  u16 inputDirection;
  u16 outputDirection;
  DiagramProperties *inputDiagram = nullptr;
  DiagramProperties *outputDiagram = nullptr;
};

struct DiagramProperty {
  u32 orientation = 0; // Pointing Left
};

class BaseProperties : public QObject {
  Q_OBJECT
public:
  explicit BaseProperties(QObject *parent = nullptr);
  virtual ~BaseProperties() {}

  quint32 id() const { return _baseProperties.id; }; //  Unique object id
  bool setId(const quint32 v);

  DiagramType::Type type() const { return _baseProperties.type; }
  bool setType(const DiagramType::Type v);

  const PeppRect &key() const { return _baseProperties.key; }
  bool setKey(const PeppRect &v);
  const PeppRect &gridRectangle() const { return _baseProperties.gridRect; }
  bool setGridRectangle(const PeppRect &v);

  bool selected() const { return _baseProperties.isSelected; }
  bool setSelected(const bool v);

protected:
  BaseProperty _baseProperties;
};

class LineProperties : public BaseProperties {
  Q_OBJECT
public:
  explicit LineProperties(QObject *parent = nullptr);

  PeppPt inputPoint() const { return _properties.input; }
  bool setInputPoint(const PeppPt pt);
  PeppPt outputPoint() const { return _properties.output; }
  bool setOutputPoint(const PeppPt pt);
  u8 inputDirection() const { return _properties.inputDirection; }
  bool setInputDirection(const u16 v);
  u8 outputDirection() const { return _properties.outputDirection; }
  bool setOutputDirection(const u16 v);
  DiagramProperties *inputDiagram() const { return _properties.inputDiagram; }
  bool setInputDiagram(DiagramProperties *diagram);
  DiagramProperties *outputDiagram() const { return _properties.outputDiagram; }
  bool setOutputDiagram(DiagramProperties *diagram);

  //  When attached diagrams move, undate line hit key
  static PeppRect recalculateKey(const DiagramProperties *inputDiagram, const DiagramProperties *outputDiagram);
  static PeppRect recalculateGridRect(const DiagramProperties *inputDiagram, const DiagramProperties *outputDiagram);

  //  Called when diagrams move
  void diagramKeyChanged() {
    setKey(LineProperties::recalculateKey(_properties.inputDiagram, _properties.outputDiagram));
  }

private:
  LineProperty _properties;
};

class Pins : public QObject {

public:
  enum PinType {
    Input = 0,
    Output,
  };
  explicit Pins(const PinType type = PinType::Input, QObject *parent = nullptr);
  auto size() const { return _lines.size(); }
  bool addLine(LineProperties *line);
  bool removeLine(LineProperties *line);

  quint16 minSize() const { return _minSize; }
  void setMinSize(const quint16 size) { _minSize = size; }
  quint16 maxSize() const { return _maxSize; }
  void setMaxSize(const quint16 size);
  PinType type() const { return _type; }
  void setType(const PinType type) { _type = type; }

  const QList<LineProperties *> &lines() const { return _lines; }
  QList<LineProperties *> &lines() { return _lines; }
  const QList<PeppRect> &pins() const { return _pins; }
  QList<PeppRect> &pins() { return _pins; }

private:
  QList<LineProperties *> _lines;
  QList<PeppRect> _pins;
  quint16 _minSize = 1;
  quint16 _maxSize = 7;
  PinType _type = PinType::Input;
};

class DiagramProperties : public BaseProperties {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(quint32 id READ id CONSTANT) // Read only
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QString imageSource READ imageSource WRITE setImageSource NOTIFY imageChanged)
  Q_PROPERTY(DiagramType::Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(quint16 inputNo READ inputNo WRITE setInputNo NOTIFY inputChanged)
  Q_PROPERTY(quint16 outputNo READ outputNo WRITE setOutputNo NOTIFY outputChanged)
  Q_PROPERTY(quint16 orientation READ orientation WRITE setOrientation NOTIFY imageChanged)

public:
  explicit DiagramProperties(QObject *parent = nullptr);

  //  Data functions
  quint16 inputNo() const { return _inputPins.maxSize(); }
  quint16 outputNo() const { return _outputPins.maxSize(); }
  void setId(const quint32 v);
  void setType(const DiagramType::Type v);
  void setInputNo(const quint16 v);
  void setOutputNo(const quint16 v);

  PeppPt input() const;
  PeppPt output() const;
  // LineProperties *outputPoint() const { return _output; }
  void setOutputPoint(LineProperties *line) {
    _outputPins.addLine(line);
    updateOutputPinPt();
  }
  // LineProperties *inputPoint() const { return _input; }
  void setInputPoint(LineProperties *line) {
    _inputPins.addLine(line);
    updateInputPinPt();
  }

  void updateInputKey(LineProperties *line) {
    if (line != nullptr) {
      //  Input is 180 degrees from output
      line->diagramKeyChanged();
    }
  }

  void updateOutputKey(LineProperties *line) {
    if (line != nullptr) {
      line->diagramKeyChanged();
    }
  }

  // Display functions
  QString name() const { return _name; }
  QString imageSource() const { return _imageSrc; }
  QPixmap *image() const { return _pixMap; }

  void setName(const QString v);
  void setImageSource(const QString v);

  // bool selected() const { return _isSelected; }
  void setSelected(const bool v);
  void setImage(QPixmap *v);

  int orientation() const { return _properties.orientation; }
  void setOrientation(const quint32 v);

  void setKey(const PeppRect &v);
  void setGridRectangle(const PeppRect &v);

  //  Pin logic
  void updateInputPinPt();
  void updateOutputPinPt();

  auto inputPins() const { return _inputPins.pins(); }
  auto outputPins() const { return _outputPins.pins(); }
signals:
  void typeChanged();
  void nameChanged();
  void imageChanged();
  void inputChanged();
  void outputChanged();
  void selectedChanged();
  void dimensionsChanged();

private:
  DiagramProperty _properties;

  //  Display properties properties
  QString _name;
  QString _imageSrc;

  //  Presentation variables that require Qt stay in this class
  QPixmap *_pixMap = nullptr;

  Pins _inputPins;
  Pins _outputPins;

  i16 _margin = 4;
};
