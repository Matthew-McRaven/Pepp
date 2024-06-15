#pragma once
#include <elfio/elfio.hpp>
#include "./helpers_globals.hpp"
#include "asm/pas/ast/node.hpp"
#include "help/builtins/book.hpp"
#include "macro/registry.hpp"

namespace helpers {
HELPERS_EXPORT QSharedPointer<const builtins::Book> book(int ed);
HELPERS_EXPORT QSharedPointer<macro::Registry> registry(QSharedPointer<const builtins::Book> book,
                                                        QStringList directory);
HELPERS_EXPORT void addMacro(macro::Registry &registry, std::string directory, QString arch);
HELPERS_EXPORT void addMacros(macro::Registry &registry, const std::list<std::string> &dirs, QString arch);

class HELPERS_EXPORT AsmHelper {
public:
  AsmHelper(QSharedPointer<macro::Registry> registry, QString os);
  void setUserText(QString user);
  bool assemble();
  QStringList errors();
  QSharedPointer<ELFIO::elfio> elf(std::optional<QList<quint8>> userObj = std::nullopt);
  QStringList listing(bool os);
  QStringList formattedSource(bool os);
  QList<quint8> bytes(bool os);

private:
  QSharedPointer<macro::Registry> _reg;
  QString _os;
  std::optional<QString> _user = std::nullopt;

  QSharedPointer<pas::ast::Node> _osRoot, _userRoot;
  QSharedPointer<ELFIO::elfio> _elf;
};

} // namespace helpers
