#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE
#include <QObject>
#include <QRect>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "common_types.hpp"
#include "core/math/geom/rectangle.hpp"

#include "diagramtype.hpp"

//  Forward declarations
class DiagramProperties;

struct BaseProperty {
  bool isSelected = false;
};

struct LineProperty {};

struct DiagramProperty {
};

class BaseProperties : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)

public:
  explicit BaseProperties(QObject *parent = nullptr);
  virtual ~BaseProperties() {}

  bool selected() const { return _baseProperties.isSelected; }
  bool setSelected(const bool v);
signals:
  void selectedChanged();

protected:
  BaseProperty _baseProperties;
};

class LineProperties : public BaseProperties {
  Q_OBJECT
public:
  explicit LineProperties(QObject *parent = nullptr);

private:
  LineProperty _properties;
};

class DiagramProperties : public BaseProperties {
  Q_OBJECT
  QML_NAMED_ELEMENT(DiagramProperties)
public:
  explicit DiagramProperties(QObject *parent = nullptr);

private:
  DiagramProperty _properties;

  i16 _margin = 4;
};
