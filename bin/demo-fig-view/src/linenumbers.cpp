// Derived from: https://github.com/andeplane/QML-Code-editor
// Code is GPL-3.0.
// Copied from 590002099bc5024a99149ca71e0ae357deadf871 on 2023-11-12
/*
 * Copyright 2016 Anders Hafreager
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "linenumbers.h"
#include <QDebug>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <QRegularExpression>

LineNumbers::LineNumbers(QQuickPaintedItem *parent) : QQuickPaintedItem(parent)
{
}

int LineNumbers::lineCount() const
{
    return m_lineCount;
}

void LineNumbers::setLineCount(int lineCount)
{
    if (m_lineCount == lineCount)
        return;

    m_lineCount = lineCount;
    emit lineCountChanged(lineCount);
}

void LineNumbers::setLineHeight(float lineHeight)
{
    if (m_lineHeight == lineHeight)
        return;

    m_lineHeight = lineHeight;
    emit lineHeightChanged(lineHeight);
}


void LineNumbers::paint(QPainter *painter)
{
    painter->save();
    // Fill entire area with grary rather than line-by-line.
    // Line-by-line can cause single pixel white gaps.
    QRectF rect(0,0,width(),height());
    painter->setPen(Qt::lightGray);
    //painter->drawRect(rect);
    painter->fillRect(rect, Qt::lightGray);
    QFont font("Courier New", 8);
    QFontMetrics fm(font);

    for(int i=0; i<m_lineCount; i++) {
        int lineNumber = i+1;

        QString text = QString("%1").arg(lineNumber);
        int textWidth = fm.horizontalAdvance(text);
        int textHeight = fm.height();
        float x = width()-textWidth*1.2;
        // Center text verticially in line by adjusting for the differences in font.
        float y = i*m_lineHeight + (.5*textHeight);
        QRectF textRect(x,y,textWidth,textHeight);

        // Dummy code to demonstrate selection vs cursor
        if(this->m_cursorPos == i) {
            painter->setPen(Qt::red);
        } else if (i >= this->m_selStart && i <= this->m_selEnd) {
            painter->setPen(Qt::green);
        } else {
            painter->setPen(Qt::black);
        }

        painter->setFont(font);
        painter->drawText(textRect, text, {});

    }
    painter->restore();
}

float LineNumbers::lineHeight() const
{
    return m_lineHeight;
}

int LineNumbers::cursorPosition() const
{
    return m_cursorPos;
}
int LineNumbers::selectionStart() const
{
    return m_selStart;
}
int LineNumbers::selectionEnd() const
{
    return m_selEnd;
}
void LineNumbers::setCursorPosition(int cursorPosition)
{
    if (m_cursorPos == cursorPosition)
        return;

    m_cursorPos = cursorPosition;
    emit cursorPositionChanged(cursorPosition);
}

void LineNumbers::setSelectionStart(int selectionStart)
{
    if (m_selStart == selectionStart)
        return;

    m_selStart = selectionStart;
    emit selectionStartChanged(selectionStart);
}

void LineNumbers::setSelectionEnd(int selectionEnd)
{
    if (m_selEnd == selectionEnd)
        return;

    m_selEnd = selectionEnd;
    emit selectionEndChanged(selectionEnd);
}
