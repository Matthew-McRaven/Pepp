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
#include "sequenceconverter.hpp"

utils::SequenceConverter::SequenceConverter(QObject *parent) : QObject{parent} {}

QString utils::SequenceConverter::toNativeText(const QVariant &sequence) {
  QKeySequence ks;
  if (auto asI = sequence; asI.canConvert<int>() && asI.convert((QMetaType)QMetaType::Int)) {
    ks = QKeySequence(static_cast<QKeySequence::StandardKey>(asI.value<int>()));
  } else if (auto asS = sequence; asS.canConvert<QString>() && asS.convert((QMetaType)QMetaType::QString)) {
    ks = QKeySequence(sequence.value<QString>());
  } else if (auto asL = sequence; asL.canConvert<QVariantList>() && asL.convert((QMetaType)QMetaType::QVariantList)) {
    QVariantList asList = asL.toList();
    if (asList.length() >= 1) return toNativeText(asList[0]);
    else return "";
  } else return "";
  return ks.toString(QKeySequence::NativeText);
}
