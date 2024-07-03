#include "asmb.hpp"
#include <iostream>
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/obj/pep10.hpp"
#include "asm/pas/operations/generic/addr2line.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "asm/pas/operations/pepp/bytes.hpp"
#include "asm/pas/operations/pepp/string.hpp"
#include "help/builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/parse.hpp"

QSharedPointer<const builtins::Book> helpers::book(int ed) {
  QString bookName;
  switch (ed) {
  case 4: bookName = "Computer Systems, 4th Edition"; break;
  case 5: bookName = "Computer Systems, 5th Edition"; break;
  case 6: bookName = "Computer Systems, 6th Edition"; break;
  default: return nullptr;
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);
  return book;
}

void helpers::addMacro(::macro::Registry &registry, std::string directory, QString arch) {
  QDirIterator it(QString::fromStdString(directory), {"*.pepm"}, QDir::Files);
  while (it.hasNext()) {
    QFile macroFile(it.next());
    if (macroFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString macroContents = macroFile.readAll();
      auto macroExpanded = ::macro::analyze_macro_definition(macroContents);
      if (!std::get<0>(macroExpanded)) continue;
      auto macroBody = macroContents.sliced(macroContents.indexOf("\n"));
      auto macro = QSharedPointer<::macro::Parsed>::create(std::get<1>(macroExpanded), std::get<2>(macroExpanded),
                                                           macroBody, arch);
      registry.registerMacro(::macro::types::User, macro);
    }
  }
}

void helpers::addMacros(::macro::Registry &registry, const std::list<std::string> &dirs, QString arch) {
  for (auto &dir : dirs) addMacro(registry, dir, arch);
}

helpers::AsmHelper::AsmHelper(QSharedPointer<::macro::Registry> registry, QString os) : _reg(registry), _os(os) {}

void helpers::AsmHelper::setUserText(QString user) { _user = user; }

bool helpers::AsmHelper::assemble() {
  QList<QPair<QString, pas::driver::pep10::Features>> targets = {{{_os, {.isOS = true}}}};
  if (_user) targets.push_back({*_user, {.isOS = false}});
  auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(targets, _reg);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

  auto osTarget = pipeline->pipelines[0].first;
  _osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  if (_user) {
    auto userTarget = pipeline->pipelines[1].first;
    _userRoot = userTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  }
  return result;
}

QStringList helpers::AsmHelper::errors() {
  using ErrList = decltype(pas::ops::generic::collectErrors(*_osRoot));
  bool hadOsErr = false;
  auto osErrors = _osRoot.isNull() ? ErrList{} : pas::ops::generic::collectErrors(*_osRoot);
  ErrList userErrors = _user && !_userRoot.isNull() ? pas::ops::generic::collectErrors(*_userRoot) : ErrList{};
  QStringList ret;
  if (!osErrors.empty()) {
    ret << "OS Errors:\n";
    auto splitOS = _os.split("\n");
    for (const auto &err : osErrors) {
      ret << ";Line " << QString::number(err.first.value.line + 1) << "\n";
      ret << splitOS[err.first.value.line] << " ;ERROR: " << err.second.message << "\n";
    }
    if (_user) ret << "User Errors:\n";
  }
  if (!userErrors.empty()) {
    auto splitUser = _user->split("\n");
    for (const auto &err : userErrors) {
      ret << ";Line " << QString::number(err.first.value.line + 1) << "\n";
      if (err.first.value.line < userErrors.size()) ret << splitUser[err.first.value.line];
      ret << " ;ERROR: " << err.second.message << "\n";
    }
  }
  return ret;
}

QList<QPair<int, QString>> helpers::AsmHelper::errorsWithLines() {
  using ErrList = decltype(pas::ops::generic::collectErrors(*_osRoot));
  bool hadOsErr = false;
  ErrList userErrors = _user && !_userRoot.isNull() ? pas::ops::generic::collectErrors(*_userRoot) : ErrList{};
  auto ret = QList<QPair<int, QString>>{};
  if (!userErrors.empty()) {
    auto splitUser = _user->split("\n");
    for (const auto &err : userErrors) {
      ret.push_back({err.first.value.line, err.second.message});
    }
  }
  return ret;
}

QSharedPointer<ELFIO::elfio> helpers::AsmHelper::elf(std::optional<QList<quint8>> userObj) {
  _elf = pas::obj::pep10::createElf();
  pas::obj::pep10::combineSections(*_osRoot);
  pas::obj::pep10::writeOS(*_elf, *_osRoot);
  if (_userRoot) {
    pas::obj::pep10::combineSections(*_userRoot);
    pas::obj::pep10::writeUser(*_elf, *_userRoot);
  } else if (userObj) {
    pas::obj::pep10::writeUser(*_elf, *userObj);
  }
  return _elf;
}

QStringList helpers::AsmHelper::listing(bool os) {
  try {
    if (os && !_osRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep10>(*_osRoot);
    else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

QList<QPair<QString, QString>> helpers::AsmHelper::splitListing(bool os) {
  try {
    if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep10>(*_osRoot);
    else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

QStringList helpers::AsmHelper::formattedSource(bool os) {
  try {
    if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep10>(*_osRoot);
    else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

QList<quint8> helpers::AsmHelper::bytes(bool os) {
  try {
    if (os && !_osRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep10>(*_osRoot);
    if (!os && !_userRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

helpers::AsmHelper::Lines2Addresses helpers::AsmHelper::address2Lines(bool os) {
  if (os && !_osRoot.isNull()) {
    auto source = pas::ops::generic::source2addr(*_osRoot);
    auto list = pas::ops::generic::list2addr(*_osRoot);
    return Lines2Addresses{source, list};
  } else if (!os && !_userRoot.isNull()) {
    auto source = pas::ops::generic::source2addr(*_userRoot);
    auto list = pas::ops::generic::list2addr(*_userRoot);
    return Lines2Addresses{source, list};
  }
  return {};
}

QSharedPointer<macro::Registry> helpers::registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros()) macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}

helpers::AsmHelper::Lines2Addresses::Lines2Addresses(QList<QPair<int, quint32>> source,
                                                     QList<QPair<int, quint32>> list) {
  for (auto [line, addr] : source) {
    _source2Addr[line] = addr;
    _addr2Source[addr] = line;
  }
  for (auto [line, addr] : list) {
    _list2Addr[line] = addr;
    _addr2List[addr] = line;
  }
}

std::optional<quint32> helpers::AsmHelper::Lines2Addresses::source2Address(int sourceLine) {
  if (_source2Addr.contains(sourceLine)) return _source2Addr[sourceLine];
  return std::nullopt;
}

std::optional<quint32> helpers::AsmHelper::Lines2Addresses::list2Address(int listLine) {
  if (_list2Addr.contains(listLine)) return _list2Addr[listLine];
  return std::nullopt;
}

std::optional<int> helpers::AsmHelper::Lines2Addresses::address2Source(quint32 address) {
  if (_addr2Source.contains(address)) return _addr2Source[address];
  return std::nullopt;
}

std::optional<int> helpers::AsmHelper::Lines2Addresses::address2List(quint32 address) {
  if (_addr2List.contains(address)) return _addr2List[address];
  return std::nullopt;
}

std::optional<int> helpers::AsmHelper::Lines2Addresses::source2List(int source) {
  auto addr = source2Address(source);
  if (!addr) return std::nullopt;
  return address2List(*addr);
}

std::optional<int> helpers::AsmHelper::Lines2Addresses::list2Source(int list) {
  auto addr = list2Address(list);
  if (!addr) return std::nullopt;
  return source2List(*addr);
}
