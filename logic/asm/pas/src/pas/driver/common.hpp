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
struct ParseResult {
  bool hadError;
  QSharedPointer<pas::ast::Node> root;
  QStringList errors;
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
  static const inline QString name = u"source_text"_qs;
  QString value;
};
struct Nodes {
  static const inline QString name = u"ast_node_list"_qs;
  QList<QSharedPointer<ast::Node>> value;
};
struct Object {
  static const inline QString name = u"object_code_list"_qs;
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
  virtual bool operator()(QSharedPointer<Globals>,
                          QSharedPointer<Target<stage>>) = 0;
  virtual stage toStage() = 0;
};

template <typename stage> struct Pipeline {
  QSharedPointer<Globals> globals;
  using target_ptr = QSharedPointer<Target<stage>>;
  using transform_list = QList<QSharedPointer<Transform<stage>>>;
  QList<QPair<target_ptr, transform_list>> pipelines;
  bool assemble();
};

// Maybe transforms should be a list / graph?
// How can I have the OS go down a different path than the user program? -- Each
// target has its own pipeline How do I insert passess with side effects that do
// not change the stage? -- Transforms can start and end in the same stage.
template <typename stage> bool Pipeline<stage>::assemble() {
  for (auto &[target, ops] : this->pipelines) {
    for (auto &op : ops) {
      if (op(globals, target))
        target->stage = op->toStage();
      else
        return false;
    }
  }
  return true;
}

} // namespace pas::driver

Q_DECLARE_METATYPE(pas::driver::repr::Source);
Q_DECLARE_METATYPE(pas::driver::repr::Nodes);
Q_DECLARE_METATYPE(pas::driver::repr::Object);
