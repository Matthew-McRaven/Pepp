#pragma once
#include <QGuiApplication>
#include <QKeySequence>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "utils_global.hpp"

namespace utils {
class UTILS_EXPORT SequenceConverter : public QObject {
  Q_OBJECT
public:
  explicit SequenceConverter(QObject *parent = nullptr);
  Q_INVOKABLE QString toNativeText(const QVariant &sequence);

signals:
};
} // namespace utils
