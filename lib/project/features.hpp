// Additional options requested for a project.
// A particular (arch, level) tuple may only support a subset of features.
// TODO: Wrap in a Q_OBJECT to expose to QML.

/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include <QString>
#include <QtCore>
#include <QtQmlIntegration>
#include "core/integers.h"

// Must be in separate file to prevent circuluar include in Qt MOC.
namespace pepp {

class FeatureHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Features)
  QML_UNCREATABLE("Error:Only enums")

public:
  enum class Features : i32 {
    None = 0,
    OneByte = 1,
    TwoByte = 2,
    NoOS = 4,
  };
  Q_ENUM(Features)
  FeatureHelper(QObject *parent = nullptr);
  Q_INVOKABLE QString string(Features abstraction) const;
};
// Tag to enable bitwise ops on enum constant
consteval void is_bitflags(FeatureHelper::Features);
using Features = FeatureHelper::Features;
QString featuresAsPrettyString(FeatureHelper::Features abstraction);
Features parseFeatures(const QString &str);
} // namespace pepp
