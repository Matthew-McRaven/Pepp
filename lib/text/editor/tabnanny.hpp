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
#include <QObject>
#include <QQuickTextDocument>
#include <QTextDocument>

// Helper class to do in/out denting of text.
class TabNanny : public QObject {
  Q_OBJECT
  Q_PROPERTY(QQuickTextDocument *document WRITE setDocument)
public:
  explicit TabNanny(QObject *parent = nullptr);
  ~TabNanny() override = default;
public slots:
  void setDocument(QQuickTextDocument *doc);
  Q_INVOKABLE void tab(int position);
  Q_INVOKABLE void backtab(int position);

private:
  QTextDocument *_doc = nullptr;
};
