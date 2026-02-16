#pragma once
#include <QObject>
#include <QRect>
#include <qqmlintegration.h>

namespace pepp {
static const int OVERLAY_NONE = 0;
static const int OVERLAY_CLOCK = 1;
static const int OVERLAY_TRISTATE = 2;
class QMLOverlay : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(QMLOverlay)
  Q_PROPERTY(QRect location READ location CONSTANT)
  Q_PROPERTY(int type READ type CONSTANT)
  QML_UNCREATABLE("TristateOverlay is only creatable from C++")
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
  QML_UNCREATABLE("TristateOverlay is only creatable from C++")
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
  QML_UNCREATABLE("TristateOverlay is only creatable from C++")
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
} // namespace pepp
