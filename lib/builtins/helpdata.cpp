#include "helpdata.hpp"
#include <builtins/figure.hpp>
#include <builtins/registry.hpp>
#include "helpmodel.hpp"

QSharedPointer<HelpEntry> about_root() {
  // relative to this the directroy in which HelpRoot.qml is located.
  return QSharedPointer<HelpEntry>::create(HelpCategory::Category::About, 0, "About", "../about/About.qml");
}

QSharedPointer<HelpEntry> writing_root() {
  auto microcode10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Microcode", "MDText.qml");
  microcode10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_mc.md"_qs)}};
  auto machine10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Machine Language", "MDText.qml");
  machine10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_oc.md"_qs)}};
  auto assembly10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Assembly Language", "MDText.qml");
  assembly10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_asmb.md"_qs)}};
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Writing Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_progs.md"_qs)}},
  root->addChildren({microcode10_language, machine10_language, assembly10_language});
  return root;
}

QSharedPointer<HelpEntry> debugging_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Debugging Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  return root;
}

QSharedPointer<HelpEntry> systemcalls_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Writing System Calls", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  return root;
}

QSharedPointer<HelpEntry> greencard10_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Setting the C bit on subtraction",
                                                 "MDText.qml");
  c_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  auto n_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0,
                                                 "Setting the N bit on <mono>CPr</mono>", "MDText.qml");
  n_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/n_bit.md"_qs)}};
  auto addr = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Addressing Modes", "MDText.qml");
  addr->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  auto reg = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Register field", "MDText.qml");
  reg->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  auto mmio = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Memory-mapped IO", "MDText.qml");
  mmio->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  auto alu = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "ALU Functions", "MDText.qml");
  alu->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_qs)}};
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, 0, "Pep/10 Reference", "ISA");
  root->addChildren({c_bit, n_bit, addr, reg, mmio, alu});
  // TODO: probably need to add props...
  return root;
}

QSharedPointer<HelpEntry> examples_root() {
  static builtins::Registry reg(nullptr);
  auto books = reg.books();
  QList<QSharedPointer<HelpEntry>> children;
  for (const auto &book : books) {
    for (const auto &figure : book->figures()) {
      auto title = u"%1 %2.%3"_qs.arg(figure->prefix(), figure->chapterName(), figure->figureName());
      auto entry =
          QSharedPointer<HelpEntry>::create(HelpCategory::Category::Figure, 0, title, "../builtins/Figure2.qml");
      entry->props = QVariantMap{{"title", title}, {"payload", QVariant::fromValue(figure.data())}};
      children.push_back(entry);
    }
  }
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, 0, "Figures", "ISA");
  root->addChildren(children);
  return root;
}

QSharedPointer<HelpEntry> problems_root() { return {}; }

QSharedPointer<HelpEntry> os_root() { return {}; }
