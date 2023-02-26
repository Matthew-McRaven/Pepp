#pragma once
#include <QtCore>
namespace pas::ast::generic {
struct Message {
  enum class Severity {
    Info,
    Debug,
    Warn,
    Fatal,
  } severity;
  QString message;
};

struct Error {
  static const inline QString attributeName = u"generic:error"_qs;
  QList<Message> value = {};
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Error);
