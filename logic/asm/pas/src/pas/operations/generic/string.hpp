#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
}

struct PAS_EXPORT SourceOptions {
  bool printErrors = false;
};
struct PAS_EXPORT ListingOptions {
  SourceOptions source;
  quint16 bytesPerLine = 3;
};

namespace pas::ops::generic::detail {
QString PAS_EXPORT formatErrorsAsComments(const ast::Node &node);
QString PAS_EXPORT format(QString symbol, QString invoke, QStringList args,
               QString comment);
QString PAS_EXPORT formatDirectiveOrMacro(const ast::Node &node, QString invoke,
                               SourceOptions opts);
QString PAS_EXPORT formatDirective(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatMacro(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatBlank(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatComment(const ast::Node &node, SourceOptions opts);
} // namespace pas::ops::generic::detail
