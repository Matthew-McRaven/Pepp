#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "schematic/component.hpp"

class CircuitProject;

//  This class is purely a wrapper for schematic::component. None of
//  schematic::component's properties are visible to QML. This class allows
//  QML access for changing properties of the currently selected component.
class ComponentWrapper : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(ComponentWrapper)
  Q_PROPERTY(quint32 id READ id NOTIFY componentChanged) // Read only
  // Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  // Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(quint16 inputNo READ inputNo /*WRITE setInputNo*/ NOTIFY inputChanged)
  Q_PROPERTY(quint16 outputNo READ outputNo /*WRITE setOutputNo*/ NOTIFY outputChanged)
  // Q_PROPERTY(quint16 orientation READ orientation WRITE setOrientation NOTIFY imageChanged)

  Component *_component = nullptr;

public:
  explicit ComponentWrapper(QObject *parent = nullptr);

  //  Set component being wrapped
  Component *component() const { return _component; }
  void setComponent(Component *_component = nullptr);

  quint32 id() const; //  Unique object id
  // QString name() const { return _name; }
  // int type() const { return static_cast<int>(_type); }
  quint16 inputNo() const;
  quint16 outputNo() const;

  // void setName(const QString v);
  // void setType(const int v);
  void setInputNo(const quint16 v);
  void setOutputNo(const quint16 v);

signals:
  void componentChanged();
  // void typeChanged();
  // void nameChanged();
  void inputChanged();
  void outputChanged();
};
