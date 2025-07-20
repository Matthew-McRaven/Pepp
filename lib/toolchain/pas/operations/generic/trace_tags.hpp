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
#include <QtCore>
#include <zpp_bits.h>
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"

namespace pas::ops::generic {

struct TraceMatch {
  QString command;
  QStringList args;
  operator QString() const;
};
// Matches type declaration (i.e., #2d) or array declaration (i.e., #2d10a)
bool isTypeTag(const QStringView &str);

std::optional<std::list<TraceMatch>> parseTraceCommand(const QString &comment);

QString infer_command(const ast::Node &node, const QStringList &args);

bool is_modifier(const QString &cmd);

struct Command {
  TraceMatch command;
  std::vector<TraceMatch> modifiers;
  std::optional<quint32> address;
  std::optional<QString> symbolDecl;
  operator QString() const;
};

struct ExtractTraceTags : public pas::ops::MutatingOp<void> {
  std::list<Command> commands;
  std::list<TraceMatch> wip_commands;
  void operator()(ast::Node &node) override;
};
std::list<Command> extractTraceTags(ast::Node &node);
} // namespace pas::ops::generic
