#include "helpdata.hpp"
#include <builtins/figure.hpp>
#include <builtins/registry.hpp>
#include "helpmodel.hpp"

using namespace Qt::StringLiterals;
QSharedPointer<HelpEntry> about_root() {
  // relative to this the directroy in which HelpRoot.qml is located.
  auto ret = QSharedPointer<HelpEntry>::create(HelpCategory::Category::About, -1, "About", "../about/About.qml");
  QList<QSharedPointer<HelpEntry>> children;
  if (auto changedb = QFileInfo(":/changelog/changelog.db"); changedb.exists()) {
    children.append(
        QSharedPointer<HelpEntry>::create(HelpCategory::Category::About, -1, "Changelog", "ChangelogViewer.qml"));
  }
  if (children.size() != 0) ret->addChildren(children);
  return ret;
}

QSharedPointer<HelpEntry> writing_root() {
  int mc10 = bitmask(builtins::Architecture::PEP10, builtins::Abstraction::MC2);
  int oc10 = bitmask(builtins::Architecture::PEP10, builtins::Abstraction::ISA3);
  int as10 = bitmask(builtins::Architecture::PEP10, builtins::Abstraction::ASMB5);
  int p10 = mc10 | oc10 | as10;
  auto microcode10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, mc10, "Microcode", "MDText.qml");
  microcode10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_mc.md"_s)}};
  auto machine10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, oc10, "Machine Language", "MDText.qml");
  machine10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_oc.md"_s)}};
  auto assembly10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, as10, "Assembly Language", "MDText.qml");
  assembly10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_asmb.md"_s)}};
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, p10, "Writing Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_progs.md"_s)}},
  root->addChildren({microcode10_language, machine10_language, assembly10_language});
  return root;
}

QSharedPointer<HelpEntry> debugging_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Debugging Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  return root;
}

QSharedPointer<HelpEntry> systemcalls_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing System Calls", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  return root;
}

QSharedPointer<HelpEntry> greencard10_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Setting the C bit on subtraction",
                                                 "MDText.qml");
  c_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  auto n_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1,
                                                 "Setting the N bit on <mono>CPr</mono>", "MDText.qml");
  n_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/n_bit.md"_s)}};
  auto addr = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Addressing Modes", "MDText.qml");
  addr->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  auto reg = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Register field", "MDText.qml");
  reg->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  auto mmio = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory-mapped IO", "MDText.qml");
  mmio->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  auto alu = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "ALU Functions", "MDText.qml");
  alu->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, -1, "Pep/10 Reference", "ISA");
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
      auto title = u"%1 %2.%3"_s.arg(figure->prefix(), figure->chapterName(), figure->figureName());
      int mask = bitmask(figure->arch(), figure->level());
      auto entry =
          QSharedPointer<HelpEntry>::create(HelpCategory::Category::Figure, mask, title, "../builtins/Figure2.qml");
      entry->props = QVariantMap{
          {"title", title},
          {"payload", QVariant::fromValue(figure.data())},
      };
      children.push_back(entry);
    }
  }
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Figures", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/figures.md"_s)}};
  root->addChildren(children);
  return root;
}

QSharedPointer<HelpEntry> problems_root() { return {}; }

QSharedPointer<HelpEntry> os_root() { return {}; }

int bitmask(builtins::Architecture arch) {
  switch (arch) {
  case builtins::ArchitectureHelper::Architecture::NONE: return 0;
  case builtins::ArchitectureHelper::Architecture::PEP8: return 1 << 0;
  case builtins::ArchitectureHelper::Architecture::PEP9: return 1 << 1;
  case builtins::ArchitectureHelper::Architecture::PEP10: return 1 << 2;
  case builtins::ArchitectureHelper::Architecture::RISCV: return 1 << 3;
  default:
    static const char *const e = "Invalid architecture";
    qCritical(e);
    throw std::invalid_argument(e);
  }
}

int bitmask(builtins::Abstraction level) {
  switch (level) {
  case builtins::AbstractionHelper::Abstraction::NONE: return 0;
  case builtins::AbstractionHelper::Abstraction::MC2: return 1 << 0;
  case builtins::AbstractionHelper::Abstraction::ISA3: return 1 << 1;
  case builtins::AbstractionHelper::Abstraction::ASMB3: return 1 << 2;
  case builtins::AbstractionHelper::Abstraction::OS4: return 1 << 3;
  case builtins::AbstractionHelper::Abstraction::ASMB5: return 1 << 4;
  default:
    static const char *const e = "Invalid abstraction";
    qCritical(e);
    throw std::invalid_argument(e);
  }
}

constexpr int shift = 16;
int bitmask(builtins::Architecture arch, builtins::Abstraction level) {
  return bitmask(arch) << shift | bitmask(level);
}

bool masked(int lhs, int rhs) {
  static_assert(shift >= 0);
  static_assert(shift <= 31);
  int shift_mask = (1 << shift) - 1;
  int mask_upper = (rhs & lhs & ~shift_mask);
  int mask_lower = (rhs & lhs & shift_mask);
  return mask_upper > 0 && mask_lower > 0;
}
