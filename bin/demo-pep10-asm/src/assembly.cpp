#include "./assembly.hpp"
#include "builtins/book.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/operations/pepp/string.hpp"

void AssemblyManger::onSelectionChanged(builtins::Figure *figure) {
  qDebug() << "Selection changed";
  _active = figure;
}

void AssemblyManger::onAssemble() {
  qDebug() << "Assembly triggered";
  if (_active == nullptr)
    return;

  auto bookRegistry = builtins::Registry(nullptr);
  auto book = bookRegistry.findBook("Computer Systems, 6th Edition");
  auto registry = QSharedPointer<macro::Registry>::create();
  for (auto &macro : book->macros())
    registry->registerMacro(macro::types::Core, macro);

  auto userBody = _active->typesafeElements()["pep"]->contents;
  auto os = _active->defaultOS();
  auto osBody = os->typesafeElements()["pep"]->contents;

  auto pipe = pas::driver::pep10::pipeline(
      {{osBody, {.isOS = true}}, {userBody, {.isOS = false}}}, registry);

  Q_ASSERT(pipe->assemble(pas::driver::pep10::Stage::End));

  auto osTarget = pipe->pipelines[0].first;
  Q_ASSERT(osTarget->bodies.contains(pas::driver::repr::Nodes::name));
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  this->_osTxt = pas::ops::pepp::formatListing<isa::Pep10>(*osRoot).join("\n");
  emit osTxtChanged();

  auto userTarget = pipe->pipelines[1].first;
  Q_ASSERT(userTarget->bodies.contains(pas::driver::repr::Nodes::name));
  auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;
  this->_usrTxt =
      pas::ops::pepp::formatListing<isa::Pep10>(*userRoot).join("\n");
  emit usrTxtChanged();
}

void AssemblyManger::clearUsrTxt() {
  _usrTxt = "";
  emit usrTxtChanged();
}

void AssemblyManger::clearOsTxt() {
  _osTxt = "";
  emit osTxtChanged();
}
