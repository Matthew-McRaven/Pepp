#pragma once
#include "./common.hpp"
#include "pas/ast/node.hpp"
#include "pas/driver/common.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/parse/pepp/node_from_parse_tree.hpp"
#include "pas/parse/pepp/rules_lines.hpp"
#include <QObject>
#include <QtCore>
#include <functional>

namespace pas::driver::pepp {
Q_NAMESPACE;
enum class Stage {
  Start,
  Parse,
  IncludeMacros,
  FlattenMacros,
  PushDownSymbols,
  GroupNodes,
  RegisterExports,
  AssignAddresses,
  ExportToObject,
  End
};
Q_ENUM_NS(Stage);

template <typename ISA>
std::function<ParseResult(QString, QSharedPointer<ast::Node>)>
createParser(bool hideEnd) {
  return [hideEnd](QString text, QSharedPointer<ast::Node> parent) {
    driver::ParseResult ret;
    auto asStd = text.toStdString();
    using namespace pas::parse::pepp;
    std::vector<LineType> result;
    auto current = asStd.begin();
    auto previous = current;
    bool success = true;
    do {
      previous = current;
      success &= boost::spirit::x3::parse(current, asStd.end(),
                                          pas::parse::pepp::line, result);
    } while (previous != current && current != asStd.end());
    if (current != asStd.end()) {
      ret.hadError = true;
      ret.errors.push_back(u"Partial parse failure"_qs);
      return ret;
    }
    if (!success) {
      ret.hadError = true;
      ret.errors.push_back(u"Unspecified parse error."_qs);
      return ret;
    }
    ret.root = pas::parse::pepp::toAST<ISA>(result, parent, hideEnd);
    auto errors = ops::generic::CollectErrors();
    ast::apply_recurse(*ret.root, errors);
    ret.hadError = errors.errors.size() != 0;
    for (auto &error : errors.errors)
      ret.errors.push_back(error.second.message);
    return ret;
  };
}

} // namespace pas::driver::pepp
