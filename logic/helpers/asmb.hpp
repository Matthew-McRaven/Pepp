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
  struct Lines2Addresses {
    Lines2Addresses(){};
    Lines2Addresses(QList<QPair<int, quint32>> source, QList<QPair<int, quint32>> list);
    std::optional<quint32> source2Address(int sourceLine);
    std::optional<quint32> list2Address(int listLine);
    std::optional<int> address2Source(quint32 address);
    std::optional<int> address2List(quint32 address);
    std::optional<int> source2List(int source);
    std::optional<int> list2Source(int list);

  private:
    QMap<int, quint32> _source2Addr{}, _list2Addr{};
    QMap<quint32, int> _addr2Source{}, _addr2List{};
  };
  AsmHelper(QSharedPointer<macro::Registry> registry, QString os);
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

private:
  QSharedPointer<macro::Registry> _reg;
  QString _os;
  std::optional<QString> _user = std::nullopt;

  QSharedPointer<pas::ast::Node> _osRoot, _userRoot;
  QSharedPointer<ELFIO::elfio> _elf;
};

} // namespace helpers
