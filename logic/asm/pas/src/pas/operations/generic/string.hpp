#pragma once
#include <QtCore>
namespace pas::ast {
class Node;
}
namespace pas::ops::generic::detail {
QString format(QString symbol, QString invoke, QStringList args,
               QString comment);
QString formatDirectiveOrMacro(const ast::Node &node, QString invoke);
QString formatDirective(const ast::Node &node);
QString formatMacro(const ast::Node &node);
QString formatBlank(const ast::Node &node);
QString formatComment(const ast::Node &node);
} // namespace pas::ops::generic::detail
