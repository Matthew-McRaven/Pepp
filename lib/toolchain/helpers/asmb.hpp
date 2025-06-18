#pragma once
#include <elfio/elfio.hpp>
#include "toolchain/pas/ast/node.hpp"
#include "enums/constants.hpp"
#include "help/builtins/book.hpp"
#include "toolchain/macro/registry.hpp"
#include "sim/debug/line_map.hpp"

namespace helpers {
void addMacro(macro::Registry &registry, std::string directory, QString arch);
void addMacros(macro::Registry &registry, const std::list<std::string> &dirs, QString arch);

class AsmHelper {
public:
  AsmHelper(QSharedPointer<macro::Registry> registry, QString os, pepp::Architecture arch = pepp::Architecture::PEP10);
  void setUserText(QString user);
  bool assemble();
  QStringList errors();
  QList<QPair<int, QString>> errorsWithLines();
  QSharedPointer<ELFIO::elfio> elf(std::optional<QList<quint8>> userObj = std::nullopt);
  QStringList listing(bool os);
  QList<QPair<QString, QString>> splitListing(bool os);
  QStringList formattedSource(bool os);
  QList<quint8> bytes(bool os);
  Lines2Addresses address2Lines(bool os);
  QSet<quint16> callViaRets();

  QSharedPointer<const pas::ast::Node> userRoot() const { return _userRoot; }

private:
  pepp::Architecture _arch;
  QSharedPointer<macro::Registry> _reg;
  QString _os;
  std::optional<QString> _user = std::nullopt;

  QSharedPointer<pas::ast::Node> _osRoot, _userRoot;
  QSharedPointer<ELFIO::elfio> _elf;
  // RET's that are being abused to act like a CALL via a double push.
  QSet<quint16> _callViaRets = {};
};

} // namespace helpers
