// File: pephighlighter.h
/*
    Pep9CPU is a CPU simulator for executing microcode sequences to
    implement instructions in the instruction set of the Pep/9 computer.

    Copyright (C) 2010  J. Stanley Warford, Pepperdine University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PEPHIGHLIGHTER_H
#define PEPHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>
#include "enu.h"
QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class PepMicroHighlighter : public QSyntaxHighlighter
{
public:
    PepMicroHighlighter(PepColors::Colors colors,QTextDocument *parent = 0);
    void forceAllFeatures(bool features);
protected:
    void highlightBlock(const QString &text);

private:
    bool forcedFeatures;
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRulesOne;
    QVector<HighlightingRule> highlightingRulesTwo;
    QVector<HighlightingRule> highlightingRulesAll;
    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat oprndFormat;
    QTextCharFormat numFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat symbolFormat;
    QTextCharFormat identFormat;
    QTextCharFormat conditionalFormat;
    QTextCharFormat branchFunctionFormat;

};

#endif // PEPHIGHLIGHTER_H
