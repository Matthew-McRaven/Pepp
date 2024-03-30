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

#include "registration.hpp"

void object::registerTypes(QQmlApplicationEngine &engine) { qmlRegisterType<Validator>("edu.pepp", 1, 0, "Validator"); }

bool Validator::valid(int key) {
  static const QSet<int> valids = {
      Qt::Key_0,    Qt::Key_1, Qt::Key_2,         Qt::Key_3,      Qt::Key_4,    Qt::Key_5,     Qt::Key_6,
      Qt::Key_7,    Qt::Key_8, Qt::Key_9,         Qt::Key_A,      Qt::Key_B,    Qt::Key_C,     Qt::Key_D,
      Qt::Key_E,    Qt::Key_F, Qt::Key_Backspace, Qt::Key_Delete, Qt::Key_Left, Qt::Key_Right, Qt::Key_Up,
      Qt::Key_Down, Qt::Key_Z, Qt::Key_Space,     Qt::Key_Return, Qt::Key_Enter};
  return valids.contains(key);
}
