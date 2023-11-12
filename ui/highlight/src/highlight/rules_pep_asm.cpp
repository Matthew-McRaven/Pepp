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

#include "rules_pep_asm.h"
#include "style/types.hpp"
#include  "isa/pep10.hpp"
#include <QRegularExpression>


QList<highlight::Rule> highlight::rules_pep10_asm()
{
    auto rules = QList<highlight::Rule>();
    // For all highlighting rules, set the case sensitivity now.
    // Doing so in the highlighting function caused a huge perfomance
    // hit for large blocks of highlighted text.
    {
        QMetaEnum mnemonicEnum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
        for(int it=0; it<mnemonicEnum.keyCount(); it++) {
            auto str = u"\\b"_qs+mnemonicEnum.key(it)+u"\\b"_qs;
            auto re = QRegularExpression(str, QRegularExpression::CaseInsensitiveOption);
            rules.push_back({re, style::Types::Mnemonic});
        }
    }

    {
      QStringList dotPatterns;
      dotPatterns << "[\\.]\\bEQUATE\\b" << "[\\.]\\bASCII\\b" << "[\\.]\\bBLOCK\\b"
                  << "[\\.]\\bBURN\\b" << "[\\.]\\bBYTE\\b" << "[\\.]\\bEND\\b"
                  << "[\\.]\\bALIGN\\b" << "[\\.]\\bWORD\\b" << "[\\.]\\bADDRSS\\b";
      for (const QString &pattern : dotPatterns) {
        auto re = QRegularExpression(pattern, QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append({re, style::Types::Dot});
      }
    }

    // Selects most accented unicode characters, based on answer:
    // https://stackoverflow.com/a/26900132
    {
        auto pattern = QRegularExpression("([A-zÀ-ÖØ-öø-ÿ_][0-9A-zÀ-ÖØ-öø-ÿ_]*)(?=:)", QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append(Rule{pattern, style::Types::Symbol});
    }

    {
        auto pattern = QRegularExpression(";.*$", QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append(Rule{pattern, style::Types::Comment});
    }

    {
        auto str="((\')(?![\'])(([^\'|\\\\]){1}|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))(\'))";
        auto pattern = QRegularExpression(str, QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append(Rule{pattern, style::Types::Quoted});
    }

    {
        auto str="((\")((([^\"|\\\\])|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))*)(\"))";
        auto pattern = QRegularExpression(str, QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append(Rule{pattern, style::Types::Quoted});
    }
    return rules;
}
