#pragma once

#include <QMetaType> // Required for Q_DECLARE_METATYPE
#include <QObject>
#include <QRect>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "common_types.hpp"
#include "core/math/geom/rectangle.hpp"

#include "schematic/component.hpp"

//  Forward declarations
class DiagramProperties;

class BaseProperties : public QObject, public ComponentVisualProperties {
  Q_OBJECT
  Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)

public:
  explicit BaseProperties(QObject *parent = nullptr);
  virtual ~BaseProperties() {}

  bool selected() const { return _isSelected; }
  bool setSelected(const bool v);
signals:
  void selectedChanged();

protected:
  bool _isSelected = false;
};

class LineProperties : public BaseProperties {
  Q_OBJECT
public:
  explicit LineProperties(QObject *parent = nullptr);

private:
};

class DiagramProperties : public BaseProperties {
  Q_OBJECT
  QML_NAMED_ELEMENT(DiagramProperties)
public:
  explicit DiagramProperties(QObject *parent = nullptr);

private:

  i16 _margin = 4;
};
