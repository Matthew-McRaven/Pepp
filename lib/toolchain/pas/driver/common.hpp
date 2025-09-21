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

namespace symbol {
class Entry;
class Table;

} // namespace symbol

namespace macro {
class Registry;
} // namespace macro

namespace pas::ast {
class Node;
}

namespace pas::driver {

struct ANTLRParserTag {};

struct ParseResult {
  bool hadError;
  QSharedPointer<pas::ast::Node> root;
  QMap<size_t, QString> errors;
};
struct Globals {
  QMap<QString, QSharedPointer<symbol::Entry>> table;
  QSharedPointer<macro::Registry> macroRegistry;
  bool contains(QString symbol) const;
  QSharedPointer<symbol::Entry> get(QString symbol);
  bool add(QSharedPointer<symbol::Entry> symbol);
};

// Holds different REPResentations of a target
namespace repr {
struct Source {
  static const inline QString name = "source_text";
  QString value;
};
struct Nodes {
  static const inline QString name = "ast_node_list";
  QSharedPointer<ast::Node> value;
};
struct Object {
  static const inline QString name = "object_code_list";
  QList<void *> value;
};
} // namespace repr

template <typename Stage> struct Target {
  enum class Kind {
    User,
    OS,
  } kind;
  QSharedPointer<symbol::Table> symbolTable;
  Stage stage;
  // Contains (transformed) views of the target. Can add additional views
  // (e.g., source/listings) as long as there are no key conflicts.
  QVariantMap bodies;
};

template <typename stage> class Transform {
public:
  virtual bool operator()(QSharedPointer<Globals>, QSharedPointer<Target<stage>>) = 0;
  virtual stage toStage() = 0;
  virtual bool hadNonASTErrors() { return false; }
  virtual QMap<size_t, QString> nonASTErrors() { return {}; }
};

template <typename stage> struct Pipeline {
  QSharedPointer<Globals> globals;
  using target_ptr = QSharedPointer<Target<stage>>;
  using transform_list = QList<QSharedPointer<Transform<stage>>>;
  QList<QPair<target_ptr, transform_list>> pipelines;
  QMap<size_t, QString> lexErrors;
  bool assemble(stage target);
};

// Maybe transforms should be a list / graph?
// How can I have the OS go down a different path than the user program? -- Each
// target has its own pipeline How do I insert passess with side effects that do
// not change the stage? -- Transforms can start and end in the same stage.
template <typename stage> bool Pipeline<stage>::assemble(stage targetStage) {
  for (auto &[target, ops] : this->pipelines) {
    for (auto &op : ops) {
      // Must explicitly deref op, or will attempt to call operator() on QSharedPointer<>.
      if (op->operator()(globals, target)) target->stage = op->toStage();
      else {
        if (op->hadNonASTErrors()) {
          const auto kvs = op->nonASTErrors();
          for (const auto &[line, err] : kvs.asKeyValueRange()) lexErrors[line] = err;
        }
        return false;
      }
      if ((int)target->stage > (int)targetStage) break;
    }
  }
  return true;
}

} // namespace pas::driver

Q_DECLARE_METATYPE(pas::driver::repr::Source);
Q_DECLARE_METATYPE(pas::driver::repr::Nodes);
Q_DECLARE_METATYPE(pas::driver::repr::Object);
