/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "tabnanny.hpp"

#include <QTextCursor>

TabNanny::TabNanny(QObject *parent) : QObject(parent) {}

void TabNanny::setDocument(QQuickTextDocument *doc) {
  if (doc == nullptr)
    return;
  else
    _doc = doc->textDocument();
}

// Tab for Pep 8/9/10
void TabNanny::tab(int position) {
  if (_doc == nullptr)
    return;

  auto cursor = QTextCursor(_doc), editCursor = QTextCursor(_doc);
  cursor.setPosition(position);
  editCursor.setPosition(position);

  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
  int curLinePos = editCursor.position() - cursor.position();
  int spaces = 0;
  if (curLinePos < 9) {
    spaces = 9 - curLinePos;
  } else if (curLinePos < 17) {
    spaces = 17 - curLinePos;
  } else if (curLinePos < 29) {
    spaces = 29 - curLinePos;
  } else if (curLinePos == 29) {
    spaces = 5;
  } else {
    spaces = 4 - ((curLinePos - 30) % 4);
  }
  QString string(spaces, QChar(u' '));
  editCursor.insertText(string);
}

// Backtab for Pep 8/9/10
void TabNanny::backtab(int position) {
  if (_doc == nullptr)
    return;

  auto cursor = QTextCursor(_doc), editCursor = QTextCursor(_doc);
  cursor.setPosition(position);
  editCursor.setPosition(position);

  cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
  int curLinePos = editCursor.position() - cursor.position();
  int startPos = 0;
  // Lines below set startPos in a assignment expression.
  if (curLinePos <= 9) {
    editCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, curLinePos - (startPos = 0));
  } else if (curLinePos <= 17) {
    editCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, curLinePos - (startPos = 9));
  } else if (curLinePos <= 29) {
    editCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, curLinePos - (startPos = 17));
  } else if (curLinePos <= 29 + 5) {
    editCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, curLinePos - (startPos = 29));
  } else {
    // Starting position is 4 characters backwards
    if ((curLinePos - 30) % 4 == 0) {
      startPos = curLinePos - 4;
    } else {
      startPos = curLinePos - (curLinePos - 30) % 4;
    }
    editCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, curLinePos - startPos);
  }
  // Validate that all characters from startPos to curLinePos are spaces, or back tab is useless.
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, startPos);
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, curLinePos - startPos);
  // Only remove text if it is only spaces.
  if (cursor.selectedText().simplified().isEmpty()) {
    editCursor.removeSelectedText();
  }
}
