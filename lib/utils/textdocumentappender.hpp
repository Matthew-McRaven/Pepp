#pragma once
#include <QQuickTextDocument>
#include <QTextCursor>
#include <QTextDocument>
#include <QtQmlIntegration>

class TextDocumentAppender : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(TextDocumentAppender)

public:
  explicit TextDocumentAppender(QObject *parent = nullptr);
  Q_INVOKABLE void appendText(QQuickTextDocument *document, const QString &text);
};
