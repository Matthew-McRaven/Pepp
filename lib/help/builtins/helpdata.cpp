#include "helpdata.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "helpmodel.hpp"

using namespace Qt::StringLiterals;
constexpr int shift = 16;

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
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  int mc10 = bitmask(PEP10, MC2);
  int oc10 = bitmask(PEP10, ISA3);
  int as10 = bitmask(PEP10, ASMB5);
  int p10 = mc10 | oc10 | as10;
  auto microcode10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, mc10, "Microcode", "MDText.qml");
  microcode10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_mc.md"_s)}};
  microcode10_language->isWIP = true;
  auto machine10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, oc10, "Machine Language", "MDText.qml");
  machine10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_oc.md"_s)}};
  machine10_language->isWIP = true;
  auto assembly10_language =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, as10, "Assembly Language", "MDText.qml");
  assembly10_language->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_asmb.md"_s)}};
  assembly10_language->isWIP = true;
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, p10, "Writing Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_progs.md"_s)}},
  root->addChildren({microcode10_language, machine10_language, assembly10_language});
  root->isWIP = true;
  return root;
}

QSharedPointer<HelpEntry> debugging_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Debugging Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  root->isWIP = true;
  return root;
}

QSharedPointer<HelpEntry> systemcalls_root() {
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing System Calls", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/debugging_progs.md"_s)}};
  root->isWIP = true;
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
  auto root =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, -1, "Pep/10 Reference", "Greencard.qml");
  QVector<QSharedPointer<HelpEntry>> children{c_bit, n_bit, addr, reg, mmio, alu};
  for (auto &c : children) c->isWIP = true;
  root->addChildren(children);
  root->isWIP = true;
  // TODO: probably need to add props...
  return root;
}

QString lexerLang(pepp::Architecture arch, pepp::Abstraction level) {
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  QString archStr = "", levelStr = "";
  switch (arch) {
  case PEP9: archStr = "Pep/9"; break;
  case PEP10: archStr = "Pep/10"; break;
  default: return "";
  }
  switch (level) {
  case ASMB3: levelStr = "ASM"; break;
  case OS4: levelStr = "ASM"; break;
  case ASMB5: levelStr = "ASM"; break;
  default: return "";
  }
  return QStringLiteral("%1 %2").arg(archStr, levelStr);
}

QString removeLeading0(const QString &str) {
  for (int it = 0; it < str.size(); it++) {
    if (str.at(it) != '0') return str.mid(it);
  }
  // Should be unreacheable, but here for safety.
  return str;
}

QSharedPointer<HelpEntry> examples_root(const builtins::Registry &reg) {
  auto books = reg.books();
  QList<QSharedPointer<HelpEntry>> children;
  for (const auto &book : books) {
    for (const auto &figure : book->figures()) {
      // Skip explicitly hidden figures (like the assembler).
      if (figure->isHidden()) continue;
      static const auto pl = QStringLiteral("%1 %2.%3");
      auto displayTitle =
          pl.arg(figure->prefix(), removeLeading0(figure->chapterName()), removeLeading0(figure->figureName()));
      auto sortTitle = pl.arg(figure->prefix(), figure->chapterName(), figure->figureName());
      int mask = bitmask(figure->arch(), figure->level());
      auto entry = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Figure, mask, displayTitle,
                                                     "../builtins/Figure2.qml");
      entry->sortName = sortTitle;
      entry->props = QVariantMap{
          {"title", displayTitle},
          {"payload", QVariant::fromValue(figure.data())},
          {"lexerLang", lexerLang(figure->arch(), figure->level())},
      };
      children.push_back(entry);
    }
  }
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Figures", "MDText.qml");
  root->isExternal = reg.usingExternalFigures();
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/figures.md"_s)}};
  root->addChildren(children);
  return root;
}

QSharedPointer<HelpEntry> macros_root(const builtins::Registry &reg) {
  auto mask = bitmask(pepp::Architecture::PEP10) << shift | 0xff;
  auto books = reg.books();
  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, mask, "Macros", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/blank.md"_s)}};
  root->isExternal = reg.usingExternalFigures();

  QMap<QString, QSharedPointer<HelpEntry>> families;
  families[""] = root;
  auto addOrGet = [&families](const QString &name, int inner_mask) {
    if (!families.contains(name)) {
      families[name] = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, inner_mask, name, "MDText.qml");
      families[name]->props = QVariantMap{{"file", QVariant(u":/help/pep10/blank.md"_s)}};
    }
    return families[name];
  };

  for (const auto &book : books) {
    for (const auto &macro : book->macros()) {
      // Skip explicitly hidden macros (like the C library).
      if (macro->hidden()) continue;
      static const auto pl = QStringLiteral("%1");
      auto displayTitle = pl.arg(macro->name());
      auto sortTitle = pl.arg(macro->name());
      auto entry = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Figure, mask, displayTitle,
                                                     "../builtins/Macro.qml");
      entry->sortName = sortTitle;
      QVariantMap nested = {{"text", QVariant::fromValue(macro->body())}, {"description", ""}};
      entry->props = QVariantMap{
          {"title", displayTitle},
          {"payload", nested},
          {"lexerLang", "Pep/10 ASM"},
      };
      addOrGet(macro->family(), mask)->addChild(entry);
    }
  }
  // Hack to add the default system calls into the macro list.
  // This is not responsive to changes in the default operating system text.
  int it = 0;
  for (const QString &scall : {"DECI", "DECO", "HEXO", "STRO", "SNOP"}) {
    static const QString pl = "LDWA %1, i\nSCALL $1, $2\n";
    auto displayTitle = scall;
    auto sortTitle = u"%1 %2"_s.arg(it++).arg(scall);
    static const auto scall_mask = bitmask(pepp::Architecture::PEP10, pepp::Abstraction::ASMB5);
    auto entry = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Figure, scall_mask, displayTitle,
                                                   "../builtins/Macro.qml");
    entry->sortName = sortTitle;
    QVariantMap nested = {{"text", QVariant::fromValue(pl.arg(scall))},
                          {"description", u"The %1 system call"_s.arg(scall)}};
    entry->props = QVariantMap{
        {"title", displayTitle},
        {"payload", nested},
        {"lexerLang", "Pep/10 ASM"},
    };
    addOrGet("System Calls", scall_mask)->addChild(entry);
  }

  // Put the intersitials into the root
  for (const auto &children : families) {
    if (children == root) continue;
    root->addChild(children);
  }
  return root;
}

QSharedPointer<HelpEntry> problems_root(const builtins::Registry &reg) { return {}; }

int bitmask(pepp::Architecture arch) {
  using enum pepp::Architecture;
  switch (arch) {
  case NO_ARCH: return 0;
  case PEP8: return 1 << 0;
  case PEP9: return 1 << 1;
  case PEP10: return 1 << 2;
  case RISCV: return 1 << 3;
  default:
    static const char *const e = "Invalid architecture";
    qCritical(e);
    throw std::invalid_argument(e);
  }
}

int bitmask(pepp::Abstraction level) {
  using enum pepp::Abstraction;
  switch (level) {
  case NO_ABS: return 0;
  case MC2: return 1 << 0;
  case ISA3: return 1 << 1;
  case ASMB3: return 1 << 2;
  case OS4: return 1 << 3;
  case ASMB5: return 1 << 4;
  default:
    static const char *const e = "Invalid abstraction";
    qCritical(e);
    throw std::invalid_argument(e);
  }
}

int bitmask(pepp::Architecture arch, pepp::Abstraction level) { return bitmask(arch) << shift | bitmask(level); }

bool masked(int lhs, int rhs) {
  static_assert(shift >= 0);
  static_assert(shift <= 31);
  int shift_mask = (1 << shift) - 1;
  int mask_upper = (rhs & lhs & ~shift_mask);
  int mask_lower = (rhs & lhs & shift_mask);
  return mask_upper > 0 && mask_lower > 0;
}
