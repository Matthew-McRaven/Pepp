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
#include <QRegularExpression>

#include "rules_pep_asm.h"
#include "style/types.hpp"
#include  "isa/pep10.hpp"
#include  "isa/pep9.hpp"

// Selects most accented unicode characters, based on answer:
// https://stackoverflow.com/a/26900132
static const auto identifier_str = "([A-zÀ-ÖØ-öø-ÿ_][0-9A-zÀ-ÖØ-öø-ÿ_]*)";
const auto symbol_re = QRegularExpression(identifier_str+u"(?=:)"_qs, QRegularExpression::PatternOption::CaseInsensitiveOption);

const auto comment_re = QRegularExpression(";.*$", QRegularExpression::PatternOption::CaseInsensitiveOption);

static const auto single_quote_str="((\')(?![\'])(([^\'|\\\\]){1}|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))(\'))";
const auto single_quote_re = QRegularExpression(single_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

static const auto double_quote_str="((\")((([^\"|\\\\])|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))*)(\"))";
const auto double_quote_re = QRegularExpression(double_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

QList<highlight::Rule> highlight::rules_pep9_asm()
{
    // For all highlighting rules, set the case sensitivity now.
    // Doing so in the highlighting function caused a huge perfomance
    // hit for large blocks of highlighted text.

    auto rules = QList<highlight::Rule>();

    rules.append(Rule{symbol_re, style::Types::Symbol});

    QMetaEnum mnemonic_enum = QMetaEnum::fromType<isa::Pep9::Mnemonic>();
    QStringList mnemonics_list;
    for(int it=0; it<mnemonic_enum.keyCount(); it++)
        mnemonics_list << u"\\b"_qs+mnemonic_enum.key(it)+u"\\b"_qs;
    auto mnemonic_re = QRegularExpression(mnemonics_list.join("|"), QRegularExpression::CaseInsensitiveOption);
    rules.push_back({mnemonic_re, style::Types::Mnemonic});

    QStringList directives_list;
    auto dirs = isa::Pep9::legalDirectives();
    for (const QString &pattern : dirs)
        directives_list << "\\."+pattern+"\\b";
    auto directive_re = QRegularExpression(directives_list.join("|"), QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({directive_re, style::Types::Dot});



    rules.append(Rule{.pattern=comment_re, .style=style::Types::Comment, .reset=1});
    rules.append(Rule{single_quote_re, style::Types::Quoted});
    rules.append(Rule{double_quote_re, style::Types::Quoted});
    return rules;
}

QList<highlight::Rule> highlight::rules_pep10_asm()
{
    // For all highlighting rules, set the case sensitivity now.
    // Doing so in the highlighting function caused a huge perfomance
    // hit for large blocks of highlighted text.

    auto rules = QList<highlight::Rule>();

    rules.append(Rule{symbol_re, style::Types::Comment});

    QMetaEnum mnemonic_enum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
    QStringList mnemonics_list;
    for(int it=0; it<mnemonic_enum.keyCount(); it++)
        mnemonics_list << u"\\b"_qs+mnemonic_enum.key(it)+u"\\b"_qs;
    auto mnemonic_re = QRegularExpression(mnemonics_list.join("|"), QRegularExpression::CaseInsensitiveOption);
    rules.push_back({mnemonic_re, style::Types::Mnemonic});

    QStringList directives_list;
    auto dirs = isa::Pep10::legalDirectives();
    for (const QString &pattern : dirs)
       directives_list << "\\."+pattern+"\\b";
    auto directive_re = QRegularExpression(directives_list.join("|"), QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({directive_re, style::Types::Dot});

    rules.append({QRegularExpression(u"@"_qs+identifier_str+"\\b", QRegularExpression::PatternOption::CaseInsensitiveOption), style::Types::Dot});    
    rules.append(Rule{.pattern=comment_re, .style=style::Types::Symbol, .reset=1});
    rules.append(Rule{single_quote_re, style::Types::Quoted});
    rules.append(Rule{double_quote_re, style::Types::Quoted});
    return rules;
}


