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
#include "help/builtins/figure.hpp"
#include <QObject>
class AssemblyManger : public QObject {
  Q_OBJECT
  builtins::Figure *_active = nullptr;
  Q_PROPERTY(QString usrTxt READ usrTxt NOTIFY usrTxtChanged);
  Q_PROPERTY(QString osTxt READ osTxt NOTIFY osTxtChanged);
  QString _usrTxt = "", _osTxt = "";

public:
  AssemblyManger() = default;
  QString usrTxt() { return _usrTxt; }
  QString osTxt() { return _osTxt; }
public slots:
  void onSelectionChanged(builtins::Figure *figure);
  void onAssemble();
  void clearUsrTxt();
  void clearOsTxt();
signals:
  void osTxtChanged();
  void usrTxtChanged();
};
