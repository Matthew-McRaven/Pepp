/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "./common.hpp"
#include "asm/pas/ast/node.hpp"
#include "asm/pas/driver/common.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "asm/pas/parse/pepp/node_from_parse_tree.hpp"
#include "asm/pas/parse/pepp/rules_lines.hpp"
#include <QObject>
#include <QtCore>
#include <functional>

namespace pas::driver::pepp {

template <typename ISA, typename ParserTag>
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
