/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include <QObject>
#include "../text_globals.hpp"

class QQuickTextDocument;

namespace highlight {

namespace style {
class Map;
}
class PatternedHighlighter;

class TEXT_EXPORT QMLHighlighter : public QObject {
  Q_OBJECT
public:
  explicit QMLHighlighter(QObject *parent = nullptr);
  Q_INVOKABLE void set_styles(highlight::style::Map *styles);
  Q_INVOKABLE void set_document(QQuickTextDocument *document);
  Q_INVOKABLE void set_highlighter(QString edition, QString language);
  Q_INVOKABLE void clear_highlighter();
private slots:
  void on_styles_changed();

private:
  style::Map *_styles = nullptr;
  struct {
    QString edition = "", language = "";
  } _active;
  PatternedHighlighter *_highlighter = nullptr;
};
} // namespace highlight
