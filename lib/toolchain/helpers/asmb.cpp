#include "asmb.hpp"
#include <iostream>
#include <sstream>
#include "enums/isa/pep10.hpp"
#include "help/builtins/registry.hpp"
#include "toolchain/macro/parse.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pep9.hpp"
#include "toolchain/pas/obj/common.hpp"
#include "toolchain/pas/obj/pep10.hpp"
#include "toolchain/pas/obj/pep9.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"
#include "toolchain/pas/operations/generic/addr2line.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"
#include "toolchain/pas/operations/pepp/whole_program_sanity.hpp"

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

helpers::AsmHelper::AsmHelper(QSharedPointer<::macro::Registry> registry, QString os, pepp::Architecture arch)
    : _arch(arch), _reg(registry), _os(os) {}

void helpers::AsmHelper::setUserText(QString user) { _user = user; }

bool helpers::AsmHelper::assemble() {
  using enum pepp::Architecture;
  _callViaRets.clear();
  switch (_arch) {
  case PEP9: {
    QList<QPair<QString, pas::driver::pep9::Features>> targets = {{{_os, {.isOS = true}}}};
    if (_user) targets.push_back({*_user, {.isOS = false}});
    auto pipeline = pas::driver::pep9::pipeline<pas::driver::ANTLRParserTag>(targets, _reg);
    auto result = pipeline->assemble(pas::driver::pep9::Stage::End);
    auto osTarget = pipeline->pipelines[0].first;
    _osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    if (_user) {
      auto userTarget = pipeline->pipelines[1].first;
      _userRoot = userTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    }
    return result;
  };
  case PEP10: {
    QList<QPair<QString, pas::driver::pep10::Features>> targets = {{{_os, {.isOS = true}}}};
    if (_user) targets.push_back({*_user, {.isOS = false}});
    auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(targets, _reg);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
    auto osTarget = pipeline->pipelines[0].first;
    _osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    if (_osRoot) pas::ops::pepp::annotateRetOps<isa::Pep10>(_callViaRets, *_osRoot);
    if (_user) {
      auto userTarget = pipeline->pipelines[1].first;
      _userRoot = userTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
      if (_userRoot) pas::ops::pepp::annotateRetOps<isa::Pep10>(_callViaRets, *_userRoot);
    }
    return result;
  };
  default: throw std::runtime_error("Invalid architecture");
  }
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
  using enum pepp::Architecture;
  switch (_arch) {
  case PEP9:
    _elf = pas::obj::pep9::createElf();
    pas::obj::pep9::writeOS(*_elf, *_osRoot);
    pas::obj::common::writeLineMapping(*_elf, *_osRoot);
    pas::obj::common::writeDebugCommands(*_elf, *_osRoot);
    if (_userRoot) {
      pas::obj::pep9::writeUser(*_elf, *_userRoot);
      pas::obj::common::writeLineMapping(*_elf, *_userRoot);
      pas::obj::common::writeDebugCommands(*_elf, *_userRoot);
    } else if (userObj) pas::obj::pep9::writeUser(*_elf, *userObj);
    break;
  case PEP10:
    _elf = pas::obj::pep10::createElf();
    pas::obj::pep10::combineSections(*_osRoot);
    pas::obj::pep10::writeOS(*_elf, *_osRoot);
    pas::obj::common::writeLineMapping(*_elf, *_osRoot);
    pas::obj::common::writeDebugCommands(*_elf, *_osRoot);
    if (_userRoot) {
      pas::obj::pep10::combineSections(*_userRoot);
      pas::obj::pep10::writeUser(*_elf, *_userRoot);
      pas::obj::common::writeLineMapping(*_elf, *_userRoot);
      pas::obj::common::writeDebugCommands(*_elf, *_userRoot);
    } else if (userObj) pas::obj::pep10::writeUser(*_elf, *userObj);
    break;
  default: throw std::logic_error("Unimplemented arch");
  }
  // Save and load so that offsets will be correct.
  {
    std::stringstream s;
    _elf->save(s);
    s.seekg(0, std::ios::beg);
    _elf->load(s);
  }
  return _elf;
}

QStringList helpers::AsmHelper::listing(bool os) {
  using enum pepp::Architecture;
  try {
    switch (_arch) {
    case PEP9:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep9>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep9>(*_userRoot);
    case PEP10:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep10>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatListing<isa::Pep10>(*_userRoot);
    default: throw std::logic_error("Unimplemented arch");
    }
  } catch (std::exception &e) {
  }
  return {};
}

QList<QPair<QString, QString>> helpers::AsmHelper::splitListing(bool os) {
  using enum pepp::Architecture;
  try {
    switch (_arch) {
    case PEP9:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep9>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep9>(*_userRoot);
    case PEP10:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep10>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSplitListing<isa::Pep10>(*_userRoot);
    default: throw std::logic_error("Unimplemented arch");
    }
  } catch (std::exception &e) {
  }
  return {};
}

QStringList helpers::AsmHelper::formattedSource(bool os) {
  using enum pepp::Architecture;
  try {
    switch (_arch) {
    case PEP9:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep9>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep9>(*_userRoot);
    case PEP10:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep10>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::formatSource<isa::Pep10>(*_userRoot);
    default: throw std::logic_error("Unimplemented arch");
    }
  } catch (std::exception &e) {
  }
  return {};
}

QList<quint8> helpers::AsmHelper::bytes(bool os) {
  using enum pepp::Architecture;
  try {
    switch (_arch) {
    case PEP9:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep9>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep9>(*_userRoot);
    case PEP10:
      if (os && !_osRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep10>(*_osRoot);
      else if (!os && !_userRoot.isNull()) return pas::ops::pepp::toBytes<isa::Pep10>(*_userRoot);
    default: throw std::logic_error("Unimplemented arch");
    }
  } catch (std::exception &e) {
  }
  return {};
}

Lines2Addresses helpers::AsmHelper::address2Lines(bool os) {
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

QSet<quint16> helpers::AsmHelper::callViaRets() { return _callViaRets; }

QSharedPointer<macro::Registry> helpers::registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros()) macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}
