#pragma once
#include "builtins/book.hpp"
#include "macro/registry.hpp"
#include "pas/ast/node.hpp"
#include <elfio/elfio.hpp>

namespace detail {
QSharedPointer<const builtins::Book> book(int ed);
QSharedPointer<macro::Registry>
registry(QSharedPointer<const builtins::Book> book, QStringList directory);

class AsmHelper {
public:
  AsmHelper(QSharedPointer<macro::Registry> registry, QString os);
  void setUserText(QString user);
  bool assemble();
  QStringList errors();
  QSharedPointer<ELFIO::elfio>
  elf(std::optional<QList<quint8>> userObj = std::nullopt);
  QStringList listing(bool os);
  QList<quint8> bytes(bool os);

private:
  QSharedPointer<macro::Registry> _reg;
  QString _os;
  std::optional<QString> _user = std::nullopt;

  QSharedPointer<pas::ast::Node> _osRoot, _userRoot;
  QSharedPointer<ELFIO::elfio> _elf;
};
} // namespace detail
