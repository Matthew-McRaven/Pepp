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

#pragma once
#include <QQmlApplicationEngine>
#include <QtCore>
#include "commands/gui.hpp"

class QTextDocument;
class QQuickTextDocument;
class BlockFinder : public QObject {
  Q_OBJECT
  QTextDocument *_doc = nullptr;

public:
  BlockFinder(QObject *parent = nullptr);
  Q_INVOKABLE int find_pos(int pos);
  Q_INVOKABLE void set_document(QQuickTextDocument *doc);
};

namespace textedit {
void registerTypes(QQmlApplicationEngine &engine);
}
