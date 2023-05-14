#include "./shared.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/obj/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include "pas/operations/pepp/string.hpp"
#include <iostream>
#include "macro/parse.cpp"

QSharedPointer<const builtins::Book> detail::book(int ed) {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
    break;
  default:
    return nullptr;
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);
  return book;
}

void detail::addMacro(::macro::Registry &registry, std::string directory, QString arch)
{
  QDirIterator it(QString::fromStdString(directory), {"*.pepm"}, QDir::Files);
  while (it.hasNext()) {
    QFile macroFile(it.next());
    if (macroFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString macroContents = macroFile.readAll();
        auto macroExpanded = ::macro::analyze_macro_definition(macroContents);
        if(!std::get<0>(macroExpanded))
            continue;
        auto macroBody = macroContents.sliced(macroContents.indexOf("\n"));
        auto macro = QSharedPointer<::macro::Parsed>::create(std::get<1>(macroExpanded), std::get<2>(macroExpanded), macroBody, arch);
        registry.registerMacro(::macro::types::User, macro);
    }
  }
}

void detail::addMacros(::macro::Registry &registry, const std::list<std::string> &dirs, QString arch)
{
  for(auto & dir: dirs)
    addMacro(registry, dir, arch);
}

detail::AsmHelper::AsmHelper(QSharedPointer<::macro::Registry> registry,
                             QString os)
    : _reg(registry), _os(os) {}

void detail::AsmHelper::setUserText(QString user) { _user = user; }

bool detail::AsmHelper::assemble() {
  QList<QPair<QString, pas::driver::pep10::Features>> targets = {
      {{_os, {.isOS = true}}}};
  if (_user)
    targets.push_back({*_user, {.isOS = false}});
  auto pipeline = pas::driver::pep10::pipeline(targets, _reg);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

  auto osTarget = pipeline->pipelines[0].first;
  _osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                .value<pas::driver::repr::Nodes>()
                .value;
  if (_user) {
    auto userTarget = pipeline->pipelines[1].first;
    _userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  }
  return result;
}

QStringList detail::AsmHelper::errors() {
  using ErrList = decltype(pas::ops::generic::collectErrors(*_osRoot));
  bool hadOsErr = false;
  auto osErrors = _osRoot.isNull() ? ErrList{} : pas::ops::generic::collectErrors(*_osRoot);
  ErrList userErrors =
      _user && !_userRoot.isNull() ? pas::ops::generic::collectErrors(*_userRoot) : ErrList{};
  QStringList ret;
  if (!osErrors.empty()) {
    ret << "OS Errors:\n";
    auto splitOS = _os.split("\n");
    for (const auto &err : osErrors) {
      ret << ";Line " << QString::number(err.first.value.line + 1) << "\n";
      ret << splitOS[err.first.value.line] << " ;ERROR: " << err.second.message
          << "\n";
    }
    if (_user)
      ret << "User Errors:\n";
  }
  if (!userErrors.empty()) {
    auto splitUser = _user->split("\n");
    for (const auto &err : userErrors) {
      ret << ";Line " << QString::number(err.first.value.line + 1) << "\n";
      ret << splitUser[err.first.value.line]
          << " ;ERROR: " << err.second.message << "\n";
    }
  }
  return ret;
}

QSharedPointer<ELFIO::elfio>
detail::AsmHelper::elf(std::optional<QList<quint8>> userObj) {
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

QStringList detail::AsmHelper::listing(bool os) {
  try {
    if (os && !_osRoot.isNull())
      return pas::ops::pepp::formatListing<isa::Pep10>(*_osRoot);
    if (!os && !_userRoot.isNull())
      return pas::ops::pepp::formatListing<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

QList<quint8> detail::AsmHelper::bytes(bool os) {
  try {
    if (os && !_osRoot.isNull())
      return pas::ops::pepp::toBytes<isa::Pep10>(*_osRoot);
    if (!os && !_userRoot.isNull())
      return pas::ops::pepp::toBytes<isa::Pep10>(*_userRoot);
  } catch (std::exception &e) {
  }
  return {};
}

QSharedPointer<macro::Registry>
detail::registry(QSharedPointer<const builtins::Book> book,
                 QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}