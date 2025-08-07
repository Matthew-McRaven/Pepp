#include "helpdata.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "helpmodel.hpp"

using namespace Qt::StringLiterals;
constexpr int shift = 16;

QSharedPointer<HelpEntry> starting_root() {
  auto abstractions =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Levels of Abstraction", "MDText.qml");
  abstractions->props = QVariantMap{{"file", QVariant(u":/help/start/abstractions.md"_s)}};
  abstractions->isWIP = true;
  abstractions->sortName = "a";
  auto new_projects =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Creating a New Project", "MDText.qml");
  new_projects->props = QVariantMap{{"file", QVariant(u":/help/start/new_projects.md"_s)}};
  new_projects->isWIP = true;
  new_projects->sortName = "b";
  auto managing_projects =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Managing Projects", "MDText.qml");
  managing_projects->props = QVariantMap{{"file", QVariant(u":/help/start/managing_projects.md"_s)}};
  managing_projects->isWIP = true;
  managing_projects->sortName = "c";
  auto modes = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Switching Modes", "MDText.qml");
  modes->props = QVariantMap{{"file", QVariant(u":/help/start/modes.md"_s)}};
  modes->isWIP = true;
  modes->sortName = "d";
  auto hiding =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Hiding/Showing Panes", "MDText.qml");
  hiding->props = QVariantMap{{"file", QVariant(u":/help/start/pane_visibility.md"_s)}};
  hiding->isWIP = true;
  hiding->sortName = "e";
  auto settings =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Changing Settings & Colors", "MDText.qml");
  settings->props = QVariantMap{{"file", QVariant(u":/help/start/settings.md"_s)}};
  settings->isWIP = true;
  settings->sortName = "f";
  auto examples =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Textbook Examples", "MDText.qml");
  examples->props = QVariantMap{{"file", QVariant(u":/help/start/examples.md"_s)}};
  examples->isWIP = true;
  examples->sortName = "g";
  auto extensions =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "File Extensions", "MDText.qml");
  extensions->props = QVariantMap{{"file", QVariant(u":/help/start/extensions.md"_s)}};
  extensions->isWIP = true;
  extensions->sortName = "h";
  auto getting_help = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Getting Help", "MDText.qml");
  getting_help->props = QVariantMap{{"file", QVariant(u":/help/start/help.md"_s)}};
  getting_help->isWIP = true;
  getting_help->sortName = "i";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Getting Started", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/start/_root.md"_s)}};
  root->addChildren(
      {abstractions, new_projects, managing_projects, modes, hiding, settings, examples, extensions, getting_help});
  return root;
}

QSharedPointer<HelpEntry> ui_root() {
  auto converters =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Byte Converters", "MDText.qml");
  converters->props = QVariantMap{{"file", QVariant(u":/help/ui/converters.md"_s)}};
  converters->isWIP = true;
  converters->sortName = "a";

  auto objedit =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Object Code Editor", "MDText.qml");
  objedit->props = QVariantMap{{"file", QVariant(u":/help/ui/object_code.md"_s)}};
  objedit->isWIP = true;
  objedit->sortName = "b";

  auto asmedit =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language Editor", "MDText.qml");
  asmedit->props = QVariantMap{{"file", QVariant(u":/help/ui/asmb_edit.md"_s)}};
  asmedit->isWIP = true;
  asmedit->sortName = "c";

  auto asmlist = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembler Listing", "MDText.qml");
  asmlist->props = QVariantMap{{"file", QVariant(u":/help/ui/asmb_list.md"_s)}};
  asmlist->isWIP = true;
  asmlist->sortName = "d";

  auto symtab = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Symbol Table", "MDText.qml");
  symtab->props = QVariantMap{{"file", QVariant(u":/help/ui/symtab.md"_s)}};
  symtab->isWIP = true;
  symtab->sortName = "e";

  auto cpudump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "CPU Display", "MDText.qml");
  cpudump->props = QVariantMap{{"file", QVariant(u":/help/ui/cpu_pane.md"_s)}};
  cpudump->isWIP = true;
  cpudump->sortName = "f";

  auto memdump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory Display", "MDText.qml");
  memdump->props = QVariantMap{{"file", QVariant(u":/help/ui/hexdump.md"_s)}};
  memdump->isWIP = true;
  memdump->sortName = "g";

  auto stkdump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stack Trace", "MDText.qml");
  stkdump->props = QVariantMap{{"file", QVariant(u":/help/ui/stacktrace.md"_s)}};
  stkdump->isWIP = true;
  stkdump->sortName = "h";

  auto iopane =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Program Input/Output", "MDText.qml");
  iopane->props = QVariantMap{{"file", QVariant(u":/help/ui/io.md"_s)}};
  iopane->isWIP = true;
  iopane->sortName = "i";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "UI Panels", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/pep10/blank.md"_s)}};
  root->isWIP = true;
  root->addChildren({converters, objedit, asmedit, asmlist, symtab, cpudump, memdump, stkdump, iopane});

  return root;
}

QSharedPointer<HelpEntry> editing_root() {
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  int mc10 = bitmask(PEP10, MC2);
  int oc10 = bitmask(PEP10, ISA3);
  int as10 = bitmask(PEP10, ASMB5);
  int p10 = mc10 | oc10 | as10;

  auto using_macros = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Using Macros", "MDText.qml");
  using_macros->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb_using_macros.md"_s)}};
  using_macros->isWIP = true;
  using_macros->sortName = "0a";

  auto mc2 = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Microcode, Mc2", "MDText.qml");
  mc2->props = QVariantMap{{"file", QVariant(u":/help/pep10/writing_mc.md"_s)}};
  mc2->isWIP = true;
  mc2->sortName = "2";

  auto isa3 =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Machine Language, ISA3", "MDText.qml");
  isa3->props = QVariantMap{{"file", QVariant(u":/help/edit/_isa3.md"_s)}};
  isa3->isWIP = true;
  isa3->sortName = "3a";
  auto isa3_writing =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing Programs", "MDText.qml");
  isa3_writing->props = QVariantMap{{"file", QVariant(u":/help/edit/isa3_writing.md"_s)}};
  isa3_writing->isWIP = true;
  isa3_writing->sortName = "1";
  auto isa3_run = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Running Programs", "MDText.qml");
  isa3_run->props = QVariantMap{{"file", QVariant(u":/help/edit/isa3_running.md"_s)}};
  isa3_run->isWIP = true;
  isa3_run->sortName = "2";
  isa3->addChildren({isa3_writing, isa3_run});

  auto asmb3 =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language, Asmb3", "MDText.qml");
  asmb3->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb_edit.md"_s)}};
  asmb3->isWIP = true;
  asmb3->sortName = "3b";
  auto asmb3_symbols = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Symbols", "MDText.qml");
  asmb3_symbols->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb_using_symbols.md"_s)}};
  asmb3_symbols->isWIP = true;
  asmb3_symbols->sortName = "0b";
  asmb3->addChildren({using_macros, asmb3_symbols});

  auto os4 =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language, OS4", "MDText.qml");
  os4->props = QVariantMap{{"file", QVariant(u":/help/edit/_os4.md"_s)}};
  os4->isWIP = true;
  os4->sortName = "4";
  auto scalls =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing System Calls", "MDText.qml");
  scalls->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_writing_scalls.md"_s)}};
  scalls->isWIP = true;
  scalls->sortName = "4a";
  auto dot_section =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The .SECTION pseudo-op", "MDText.qml");
  dot_section->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_section.md"_s)}};
  dot_section->isWIP = true;
  dot_section->sortName = "4b";
  auto dot_export =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The .EXPORT pseudo-op", "MDText.qml");
  dot_export->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_export.md"_s)}};
  dot_export->isWIP = true;
  dot_export->sortName = "4c";
  auto dot_scall =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The .SCALL pseudo-op", "MDText.qml");
  dot_scall->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_scall.md"_s)}};
  dot_scall->isWIP = true;
  dot_scall->sortName = "4d";
  auto dot_org =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The .ORG pseudo-op", "MDText.qml");
  dot_org->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_org.md"_s)}};
  dot_org->isWIP = true;
  dot_org->sortName = "4e";

  auto traps =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing Trap Handlers", "MDText.qml");
  traps->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_writing_traps.md"_s)}};
  traps->isWIP = true;
  traps->sortName = "5a";
  auto dot_burn =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The .BURN pseudo-op", "MDText.qml");
  dot_burn->props = QVariantMap{{"file", QVariant(u":/help/edit/os4_burn.md"_s)}};
  dot_burn->isWIP = true;
  dot_burn->sortName = "5b";
  os4->addChildren({using_macros, scalls, dot_section, dot_export, dot_scall, dot_org, traps, dot_burn});

  auto asmb5 =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language, Asmb5", "MDText.qml");
  asmb5->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb_edit.md"_s)}};
  asmb5->isWIP = true;
  asmb5->sortName = "5";
  auto asmb5_writing =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Using System Calls", "MDText.qml");
  asmb5_writing->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb5_using_scalls.md"_s)}};
  asmb5_writing->isWIP = true;
  asmb5_writing->sortName = "5a";
  auto asmb5_trace =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stack Trace Tags", "MDText.qml");
  asmb5_trace->props = QVariantMap{{"file", QVariant(u":/help/edit/asmb5_trace_tags.md"_s)}};
  asmb5_trace->isWIP = true;
  asmb5_trace->sortName = "5b";
  asmb5->addChildren({using_macros, asmb5_writing, asmb5_trace});

  auto auto_format =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Automatic Formatting", "MDText.qml");
  auto_format->props = QVariantMap{{"file", QVariant(u":/help/edit/autoformat.md"_s)}};
  auto_format->isWIP = true;
  auto_format->sortName = "0";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Editing Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/edit/_root.md"_s)}};
  root->isWIP = true;
  root->addChildren({auto_format, mc2, isa3, asmb3, os4, asmb5});
  return root;
}

QSharedPointer<HelpEntry> debugging_root() {
  auto step =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stepping through Programs", "MDText.qml");
  step->props = QVariantMap{{"file", QVariant(u":/help/debug/step.md"_s)}};
  step->isWIP = true;
  step->sortName = "0";
  auto trace_isa =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Tracing Machine Language", "MDText.qml");
  trace_isa->props = QVariantMap{{"file", QVariant(u":/help/debug/isa.md"_s)}};
  trace_isa->isWIP = true;
  trace_isa->sortName = "1";
  auto trace_asmb =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Tracing Assembly Language", "MDText.qml");
  trace_asmb->props = QVariantMap{{"file", QVariant(u":/help/debug/asmb.md"_s)}};
  trace_asmb->isWIP = true;
  trace_asmb->sortName = "2";
  auto bps = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Using Breakpoints", "MDText.qml");
  bps->props = QVariantMap{{"file", QVariant(u":/help/debug/breakpoints.md"_s)}};
  bps->isWIP = true;
  bps->sortName = "3";
  auto endless = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Endless Loops", "MDText.qml");
  endless->props = QVariantMap{{"file", QVariant(u":/help/debug/loops.md"_s)}};
  endless->isWIP = true;
  endless->sortName = "4";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Debugging Programs", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/debug/_root.md"_s)}};
  root->isWIP = true;
  root->addChildren({step, trace_isa, trace_asmb, bps, endless});
  return root;
}

QSharedPointer<HelpEntry> advanced_root() {
  auto brk_view =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Viewing Breakpoints", "MDText.qml");
  brk_view->props = QVariantMap{{"file", QVariant(u":/help/advanced/view_bp.md"_s)}};
  brk_view->isWIP = true;
  brk_view->sortName = "a";
  auto watch_expr =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Watch Expressions", "MDText.qml");
  watch_expr->props = QVariantMap{{"file", QVariant(u":/help/advanced/watch_expr.md"_s)}};
  watch_expr->isWIP = true;
  watch_expr->sortName = "b";
  auto dbg_expr =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Debug Expressions", "MDText.qml");
  dbg_expr->props = QVariantMap{{"file", QVariant(u":/help/advanced/debug_expr.md"_s)}};
  dbg_expr->isWIP = true;
  dbg_expr->sortName = "c";
  auto brk_cond =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Conditional Breakpoints", "MDText.qml");
  brk_cond->props = QVariantMap{{"file", QVariant(u":/help/advanced/cond_bp.md"_s)}};
  brk_cond->isWIP = true;
  brk_cond->sortName = "d";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Advanced Topics", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/advanced/_root.md"_s)}};
  root->isWIP = true;
  root->addChildren({brk_view, watch_expr, dbg_expr, brk_cond});
  return root;
}

QSharedPointer<HelpEntry> greencard10_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "C bit on SUBr", "MDText.qml");
  c_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/c_bit.md"_s)}};
  c_bit->sortName = "001";
  auto n_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "N bit for CPBr", "MDText.qml");
  n_bit->props = QVariantMap{{"file", QVariant(u":/help/pep10/n_bit.md"_s)}};
  n_bit->sortName = "002";
  auto addr = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Addressing Modes", "MDText.qml");
  addr->props = QVariantMap{{"file", QVariant(u":/help/pep10/addr_modes.md"_s)}};
  addr->sortName = "003";
  auto reg = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Register field", "MDText.qml");
  reg->props = QVariantMap{{"file", QVariant(u":/help/pep10/register_fields.md"_s)}};
  reg->sortName = "004";
  auto alu = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "ALU Functions", "MDText.qml");
  alu->props = QVariantMap{{"file", QVariant(u":/help/pep10/alu_func.md"_s)}};
  alu->sortName = "005";
  auto mmio = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory-Mapped IO", "MDText.qml");
  mmio->props = QVariantMap{{"file", QVariant(u":/help/pep10/mmio.md"_s)}};
  mmio->sortName = "006";
  auto root =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, -1, "Pep/10 Reference", "Greencard.qml");
  QVector<QSharedPointer<HelpEntry>> children{c_bit, n_bit, addr, reg, alu, mmio};
  root->addChildren(children);
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
  for (const auto &book : std::as_const(books)) {
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
  root->props = QVariantMap{{"file", QVariant(u":/help/figures.md"_s)}};
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
