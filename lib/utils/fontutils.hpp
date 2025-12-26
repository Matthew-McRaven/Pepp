/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include <QtGui/qfont.h>
#include <QtQmlIntegration>

class FontUtilsHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")
public:
  FontUtilsHelper(QFont font, QObject *parent = nullptr);
  Q_INVOKABLE FontUtilsHelper *h1();
  Q_INVOKABLE FontUtilsHelper *h2();
  Q_INVOKABLE FontUtilsHelper *h3();
  Q_INVOKABLE FontUtilsHelper *bold();
  Q_INVOKABLE FontUtilsHelper *nobold();
  Q_INVOKABLE FontUtilsHelper *italicize();
  Q_INVOKABLE FontUtilsHelper *noitalicize();
  Q_INVOKABLE QFont font();

private:
  QFont _font;
};

class FontUtils : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  FontUtils(QObject *parent = nullptr);
  Q_INVOKABLE FontUtilsHelper *fromFont(QFont font);
};
