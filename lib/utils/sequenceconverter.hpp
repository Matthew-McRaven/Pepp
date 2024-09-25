#pragma once
#include <QGuiApplication>
#include <QKeySequence>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>

namespace utils {
class SequenceConverter : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_ELEMENT

public:
  explicit SequenceConverter(QObject *parent = nullptr);
  Q_INVOKABLE QString toNativeText(const QVariant &sequence);

signals:
};
} // namespace utils
