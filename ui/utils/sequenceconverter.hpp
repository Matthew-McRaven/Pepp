#pragma once
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QKeySequence>
#include <QQmlContext>
#include <QObject>
#include "utils_global.hpp"

namespace utils {
class UTILS_EXPORT SequenceConverter : public QObject {
  Q_OBJECT
public:
  explicit SequenceConverter(QObject *parent = nullptr);
  Q_INVOKABLE QString toNativeText(const QVariant& sequence);

signals:
};
}

