// Derived from: https://github.com/andeplane/QML-Code-editor
// Code is GPL-3.0.
// Copied from 590002099bc5024a99149ca71e0ae357deadf871 on 2023-11-12
// I've modified this code to assume constant line heights, therefore eliminating text parsing.
// I've also added helper code to translate cursors -> lines, so all cursor positions are
// now line numbers.
// Scrolling is handled externally, and thus eliminated from this class.
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
#ifndef LINENUMBERS_H
#define LINENUMBERS_H

#include <QQuickPaintedItem>

class LineNumbers : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int lineCount READ lineCount WRITE setLineCount NOTIFY lineCountChanged)
    Q_PROPERTY(float lineHeight READ lineHeight WRITE setLineHeight NOTIFY lineHeightChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged)
public:
    explicit LineNumbers(QQuickPaintedItem *parent = nullptr);
    int lineCount() const;
    float lineHeight() const;
    int cursorPosition() const;
    int selectionStart() const;
    int selectionEnd() const;
    void paint(QPainter *painter) override;

signals:
    void lineCountChanged(int lineCount);
    void lineHeightChanged(float lineHeight);
    void cursorPositionChanged(int cursorPosition);
    void selectionStartChanged(int selectionStart);
    void selectionEndChanged(int selectionEnd);

public slots:
    void setLineCount(int lineCount);
    void setLineHeight(float lineHeight);
    void setCursorPosition(int cursorPosition);
    void setSelectionStart(int selectionStart);
    void setSelectionEnd(int selectionEnd);
private:
    int m_cursorPos=0, m_selStart=0, m_selEnd=0;
    int m_lineCount = 0;
    float m_lineHeight = 0;
};

#endif // LINENUMBERS_H
