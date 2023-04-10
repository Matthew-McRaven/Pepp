#pragma once

#include "pas/ast/node.hpp"
#include <QObject>

/*struct ParseResult : public QObject {
  Q_OBJECT;
  Q_PROPERTY(bool passed READ passed CONSTANT)
  Q_PROPERTY(QStringList errors READ errors CONSTANT);
  Q_PROPERTY(QString source READ source CONSTANT);
  Q_PROPERTY(QString list READ list CONSTANT);

  ParseResult(QObject *parent);
  ParseResult(QObject *parent, QSharedPointer<const pas::ast::Node>);

public:
};

class Parser : public QObject {
  Q_OBJECT
public:
  // Ownership of parse result is transfered to caller.
  Q_INVOKABLE QList<ParseResult> *parse(QString arg);
};*/
