#pragma once
#include <QtCore>
namespace pas::ast {
class Node;
}

struct SourceOptions {
  bool printErrors = false;
};
struct ListingOptions {
  SourceOptions source;
  quint16 bytesPerLine = 3;
};

namespace pas::ops::generic::detail {
QString formatErrorsAsComments(const ast::Node &node);
QString format(QString symbol, QString invoke, QStringList args,
               QString comment);
QString formatDirectiveOrMacro(const ast::Node &node, QString invoke,
                               SourceOptions opts);
QString formatDirective(const ast::Node &node, SourceOptions opts);
QString formatMacro(const ast::Node &node, SourceOptions opts);
QString formatBlank(const ast::Node &node, SourceOptions opts);
QString formatComment(const ast::Node &node, SourceOptions opts);
} // namespace pas::ops::generic::detail
