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
#include <QSyntaxHighlighter>

#include "rules.hpp"

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace highlight {
class PatternedHighlighter : public QSyntaxHighlighter
{
public:
    struct Pattern{
        QRegularExpression pattern;
        QTextCharFormat format;
        int from=0, to=0;
        bool reset=false;
    };
    PatternedHighlighter(QObject *parent = 0);
    PatternedHighlighter(QTextDocument *parent = 0);
    void setPatterns(QList<Pattern> rules);
protected:
    void highlightBlock(const QString &text);
private:
    QList<Pattern> _rules;
};
}
