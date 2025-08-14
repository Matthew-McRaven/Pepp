#include "textdocumentappender.hpp"

TextDocumentAppender::TextDocumentAppender(QObject *parent) : QObject(parent) {}

void TextDocumentAppender::appendText(QQuickTextDocument *qdocument, const QString &text) {
  if (!qdocument) return;
  auto document = qdocument->textDocument();
  QTextCursor c(document);
  c.movePosition(QTextCursor::End);
  if (!document->isEmpty()) c.insertBlock(); // newline only when needed
  c.insertText(text);
}
