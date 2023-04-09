#include "pas/driver/pep10.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "macro/registry.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

class PasE2E_Pep10 : public QObject {
  Q_OBJECT
  QSharedPointer<macro::Registry> registry;
  void loadBookMacros(QSharedPointer<const builtins::Book> book) {
    for (auto &macro : book->macros())
      registry->registerMacro(macro::types::Core, macro);
  }
  void injectFakeSCallMacros() {
    QStringList nonunary = {"DECI", "DECO", "STRO"};
    for (auto &macro : nonunary)
      registry->registerMacro(
          macro::types::Core,
          QSharedPointer<macro::Parsed>::create(
              macro, 2, "LDWT 0,i\nSCALL $1, $2", "pep/10"));
  }

private slots:
  void standaloneUser() {
    QFETCH(QString, body);
    auto pipeline = pas::driver::pep10::pipeline(
        {{body, {.isOS = false, .ignoreUndefinedSymbols = true}}}, registry);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
    auto target = pipeline->pipelines[0].first;
    QVERIFY(target->bodies.contains(pas::driver::repr::Nodes::name));
    auto root = target->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
    // print out error messages before failing -- enables debugging broken
    // tests.
    if (!result) {
      QStringList body =
          pas::ops::pepp::formatSource<pas::isa::Pep10ISA>(*root);
      for (auto &line : body)
        qCritical() << line;
      qCritical() << "";

      for (auto &error : pas::ops::generic::collectErrors(*root))
        qCritical() << error.first.value.line << error.second.message;
    }
    QVERIFY(result);
  }

  void standaloneUser_data() {
    QTest::addColumn<QString>("body");
    auto registry = builtins::Registry(nullptr);
    auto book = registry.findBook("Computer Systems, 6th Edition");
    QVERIFY(!book.isNull());

    this->registry = QSharedPointer<macro::Registry>::create();
    loadBookMacros(book);
    injectFakeSCallMacros();

    for (auto &fig : book->figures()) {
      if (fig->isOS())
        continue;
      else if (!fig->typesafeElements().contains("pep"))
        continue;
      auto chName = fig->chapterName().toStdString();
      auto figName = fig->figureName().toStdString();
      QTest::addRow("Figure %s.%s", chName.data(), figName.data())
          << fig->typesafeElements()["pep"]->contents;
    }
  }
};

#include "pep10.test.moc"

QTEST_MAIN(PasE2E_Pep10);
