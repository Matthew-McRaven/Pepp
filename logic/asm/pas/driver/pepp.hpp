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


namespace isa { class Pep10; }
namespace pas::driver::pepp {


namespace detail {
template <typename ParserTag, typename ISA>
struct Helper {
    driver::ParseResult operator()(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd){return {};};
};

template <typename ISA>
struct Helper<ISA, pas::driver::BoostParserTag> {
    driver::ParseResult operator()(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd){
        driver::ParseResult ret = {.hadError = false};
        using namespace pas::parse::pepp;
        std::vector<LineType> result;
        auto current = input.begin();
        auto previous = current;
        bool success = true;
        do {
            previous = current;
            success &= boost::spirit::x3::parse(current, input.end(),
                                                pas::parse::pepp::line, result);
        } while (previous != current && current != input.end());
        if (current != input.end()) {
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
        return ret;
    };
};

driver::ParseResult antlr4_pep10(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd);
template <>
struct Helper<isa::Pep10, pas::driver::ANTLRParserTag> {
    driver::ParseResult operator()(const std::string& input, QSharedPointer<ast::Node> parent, bool hideEnd){
        // Convert input string to parsed lines.
        return detail::antlr4_pep10(input, parent, hideEnd);
    };
};
}


template <typename ISA, typename ParserTag>
std::function<ParseResult(QString, QSharedPointer<ast::Node>)>
createParser(bool hideEnd) {
  return [hideEnd](QString text, QSharedPointer<ast::Node> parent) {
    detail::Helper<ISA, ParserTag> helper;
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
