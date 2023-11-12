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

#include "patternedhiglighter.hpp"

using namespace highlight;

PatternedHighlighter::PatternedHighlighter(QObject *parent): QSyntaxHighlighter(parent),
    _rules()
{
}

PatternedHighlighter::PatternedHighlighter( QTextDocument *parent): QSyntaxHighlighter(parent),
    _rules()
{
}

void PatternedHighlighter::setPatterns(QList<Pattern> rules)
{
    _rules = rules;
}

void PatternedHighlighter::highlightBlock(const QString &text)
{
    auto prevState = previousBlockState();
    if(prevState == -1) prevState = 0;
    int index=0;
    for(const auto & rule : _rules) {
        if(rule.from != prevState) continue;
        else if(auto match = rule.pattern.match(text, rule.reset ? 0 : index); match.hasMatch()) {
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            setCurrentBlockState(prevState = rule.to);
            index=match.capturedEnd();
            if(index >= text.length()) break;
        }
    }
}
