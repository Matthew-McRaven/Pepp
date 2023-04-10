#pragma once
#include <QMap>
#include <QObject>

#include <builtins/registry.hpp>

#include "builtins/figure.hpp"

class FigureManager : public QObject {
  Q_OBJECT
  QMap<qsizetype, QSharedPointer<builtins::Figure>> _figureMap;
  QSharedPointer<builtins::Registry> _reg;

public:
  FigureManager();
  Q_INVOKABLE QStringList figures();
  Q_INVOKABLE builtins::Figure *figureAt(qsizetype index);
};
