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
  static auto serialize(auto &archive, auto &self) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;

    if constexpr (archive_type::kind() == zpp::bits::kind::in) {
      std::string command;
      std::vector<std::string> args;
      auto ret = archive(command, args);
      if (zpp::bits::failure(ret)) return ret;
      self.command = QString::fromStdString(command);
      self.args = {};
      for (auto &arg : args) self.args.append(QString::fromStdString(arg));
      return ret;
    } else if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      std::string out = self.command.toStdString();
      (void)archive(out);
      std::vector<std::string> args;
      for (auto &arg : self.args) args.push_back(arg.toStdString());
      return archive(args);
    }
  }
};
// Matches type declaration (i.e., #2d) or array declaration (i.e., #2d10a)
bool isTypeTag(const QStringView &str);

std::optional<std::vector<TraceMatch>> parseTraceCommand(const QString &comment);

QString infer_command(const ast::Node &node, const QStringList &args);

bool is_modifier(const QString &cmd);

struct Command {
  TraceMatch command;
  std::vector<TraceMatch> modifiers;
  std::optional<quint32> address;
  operator QString() const;
  static auto serialize(auto &archive, auto &self) { return archive(self.address, self.command, self.modifiers); }
};

struct ExtractTraceTags : public pas::ops::MutatingOp<void> {
  std::vector<Command> commands;
  std::vector<TraceMatch> wip_commands;
  void operator()(ast::Node &node) override;
};
std::vector<Command> extractTraceTags(ast::Node &node);
} // namespace pas::ops::generic
