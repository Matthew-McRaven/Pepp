/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "asm/pas/ast/node.hpp"
#include "elfio/elfio.hpp"
#include "help/builtins/book.hpp"
#include "macro/registry.hpp"

#if INCLUDE_GUI
class QQmlApplicationEngine;
// Base class for application-specific global handling.
// Needed because an init_fn has a smaller lexical scope than the calling function,
// so stack-allocated "globals" fall out of scope.
struct gui_globals {
  virtual ~gui_globals() = default;
};
// Perform any type-registration, create & bind necessary globals.
using init_fn = QSharedPointer<gui_globals> (*)(QQmlApplicationEngine &);
#endif

namespace detail {
QSharedPointer<const builtins::Book> book(int ed);
QSharedPointer<macro::Registry> registry(QSharedPointer<const builtins::Book> book, QStringList directory);
void addMacro(macro::Registry &registry, std::string directory, QString arch);
void addMacros(macro::Registry &registry, const std::list<std::string> &dirs, QString arch);

class AsmHelper {
public:
  AsmHelper(QSharedPointer<macro::Registry> registry, QString os);
  void setUserText(QString user);
  bool assemble();
  QStringList errors();
  QSharedPointer<ELFIO::elfio> elf(std::optional<QList<quint8>> userObj = std::nullopt);
  QStringList listing(bool os);
  QList<quint8> bytes(bool os);

private:
  QSharedPointer<macro::Registry> _reg;
  QString _os;
  std::optional<QString> _user = std::nullopt;

  QSharedPointer<pas::ast::Node> _osRoot, _userRoot;
  QSharedPointer<ELFIO::elfio> _elf;
};
struct SharedFlags {
  int edValue = 6;
  bool isGUI = false;
};
} // namespace detail
