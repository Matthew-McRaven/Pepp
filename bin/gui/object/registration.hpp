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
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QWidget>
#include <QWindow>
#include <QtCore>
#include "commands/gui.hpp"
#include "qapplication.h"

class KeyEmitter : public QObject {
  Q_OBJECT
public:
  explicit KeyEmitter(QObject *parent = nullptr) : QObject(parent) {}

  Q_INVOKABLE void emitRight() { emitKey(Qt::Key_Right); }

  Q_INVOKABLE void emitEnter() { emitKey(Qt::Key_Enter); }

private:
  void emitKey(Qt::Key k) {
    auto *focusedWidget = QGuiApplication::focusWindow();
    if (focusedWidget) {
      QKeyEvent *pressEvent = new QKeyEvent(QEvent::KeyPress, k, Qt::NoModifier);
      QApplication::postEvent(focusedWidget, pressEvent);

      QKeyEvent *releaseEvent = new QKeyEvent(QEvent::KeyRelease, k, Qt::NoModifier);
      QApplication::postEvent(focusedWidget, releaseEvent);
    }
  }
};

namespace object {
void registerTypes(QQmlApplicationEngine &engine);
}
