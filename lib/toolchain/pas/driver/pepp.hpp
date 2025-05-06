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
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/driver/common.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include <QObject>
#include <QtCore>
#include <functional>

namespace isa {
class Pep10;
class Pep9;
} // namespace isa

namespace pas::driver::pepp {
namespace detail {
template <typename ParserTag, typename ISA>
struct Helper {
    driver::ParseResult operator()(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd){return {};};
};

driver::ParseResult antlr4_pep10(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd);
template <> struct Helper<pas::driver::ANTLRParserTag, isa::Pep10> {
  driver::ParseResult operator()(const std::string &input, QSharedPointer<ast::Node> parent, bool hideEnd) {
    // Convert input string to parsed lines.
    return detail::antlr4_pep10(input, parent, hideEnd);
  };
};
driver::ParseResult antlr4_pep9(const std::string &input, QSharedPointer<ast::Node> parent, bool hideEnd);
template <> struct Helper<pas::driver::ANTLRParserTag, isa::Pep9> {
  driver::ParseResult operator()(const std::string &input, QSharedPointer<ast::Node> parent, bool hideEnd) {
    // Convert input string to parsed lines.
    return detail::antlr4_pep9(input, parent, hideEnd);
  };
};
}


template <typename ISA, typename ParserTag>
std::function<ParseResult(QString, QSharedPointer<ast::Node>)>
createParser(bool hideEnd) {
  return [hideEnd](QString text, QSharedPointer<ast::Node> parent) {
    detail::Helper<ParserTag, ISA> helper;
    auto asStd = text.toStdString();
    auto ret = helper(asStd, parent, hideEnd);
    if(ret.hadError) return ret;

    auto errors = ops::generic::CollectErrors();
    ast::apply_recurse(*ret.root, errors);
    ret.hadError |= errors.errors.size() != 0;
    for (auto &error : errors.errors)
        ret.errors.push_back(error.second.message);
    return ret;
  };
}

} // namespace pas::driver::pepp
