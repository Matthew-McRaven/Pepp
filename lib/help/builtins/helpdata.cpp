#include "helpdata.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "helpmodel.hpp"

using namespace Qt::StringLiterals;
constexpr int shift = 16;

QSharedPointer<HelpEntry> starting_root() {
  auto abstractions =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Levels of Abstraction", "MDText.qml");
  abstractions->props = QVariantMap{{"file", QVariant(u":/help/start/abstractions.html"_s)}};
  abstractions->sortName = "a";
  abstractions->slug = "abstract";
  auto new_projects =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Creating a New Project", "MDText.qml");
  new_projects->props = QVariantMap{{"file", QVariant(u":/help/start/new_projects.html"_s)}};
  new_projects->sortName = "b";
  new_projects->slug = "new";
  auto managing_projects =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Managing Projects", "MDText.qml");
  managing_projects->props = QVariantMap{{"file", QVariant(u":/help/start/managing_projects.html"_s)}};
  managing_projects->sortName = "c";
  managing_projects->slug = "manage";
  auto modes = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Switching Modes", "MDText.qml");
  modes->props = QVariantMap{{"file", QVariant(u":/help/start/modes.html"_s)}};
  modes->sortName = "d";
  modes->slug = "modes";
  auto hiding =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Hiding/Showing Panes", "MDText.qml");
  hiding->props = QVariantMap{{"file", QVariant(u":/help/start/pane_visibility.html"_s)}};
  hiding->sortName = "e";
  hiding->slug = "hide";
  auto settings =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Changing Settings & Colors", "MDText.qml");
  settings->props = QVariantMap{{"file", QVariant(u":/help/start/settings.html"_s)}};
  settings->sortName = "f";
  settings->slug = "settings";
  auto examples =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Textbook Examples", "MDText.qml");
  examples->props = QVariantMap{{"file", QVariant(u":/help/start/examples.html"_s)}};
  examples->sortName = "g";
  examples->slug = "ext";
  auto extensions =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "File Extensions", "MDText.qml");
  extensions->props = QVariantMap{{"file", QVariant(u":/help/start/extensions.html"_s)}};
  extensions->sortName = "h";
  extensions->slug = "extensions";
  auto getting_help = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Getting Help", "MDText.qml");
  getting_help->props = QVariantMap{{"file", QVariant(u":/help/start/help.html"_s)}};
  getting_help->sortName = "i";
  getting_help->slug = "help";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Getting Started", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/start/index.html"_s)}};
  root->slug = "start";
  root->addChildren(
      {abstractions, new_projects, managing_projects, modes, hiding, settings, examples, extensions, getting_help});
  return root;
}

QSharedPointer<HelpEntry> ui_root() {
  auto converters =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Byte Converters", "MDText.qml");
  converters->props = QVariantMap{{"file", QVariant(u":/help/ui/converters.html"_s)}};
  converters->sortName = "a";
  converters->slug = "convert";

  auto objedit =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Object Code Editor", "MDText.qml");
  objedit->props = QVariantMap{{"file", QVariant(u":/help/ui/object_code.html"_s)}};
  objedit->sortName = "b";
  objedit->slug = "obj";

  auto asmedit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Editor", "MDText.qml");
  asmedit->props = QVariantMap{{"file", QVariant(u":/help/ui/asmb.md"_s)}};
  asmedit->isWIP = true;
  asmedit->sortName = "c";
  asmedit->slug = "asmb";

  auto symtab = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Symbol Table", "MDText.qml");
  symtab->props = QVariantMap{{"file", QVariant(u":/help/ui/symtab.html"_s)}};
  symtab->isWIP = true;
  symtab->sortName = "e";
  symtab->slug = "sym";

  auto cpudump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "CPU Display", "MDText.qml");
  cpudump->props = QVariantMap{{"file", QVariant(u":/help/ui/cpu_pane.html"_s)}};
  cpudump->sortName = "f";
  cpudump->slug = "cpu";

  auto memdump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory Display", "MDText.qml");
  memdump->props = QVariantMap{{"file", QVariant(u":/help/ui/hexdump.html"_s)}};
  memdump->sortName = "g";
  memdump->slug = "mem";

  auto stkdump = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stack Trace", "MDText.qml");
  stkdump->props = QVariantMap{{"file", QVariant(u":/help/ui/stacktrace.md"_s)}};
  stkdump->isWIP = true;
  stkdump->sortName = "h";
  stkdump->slug = "stack";

  auto iopane =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Program Input/Output", "MDText.qml");
  iopane->props = QVariantMap{{"file", QVariant(u":/help/ui/io.md"_s)}};
  iopane->isWIP = true;
  iopane->sortName = "i";
  iopane->slug = "io";

  auto root =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "User Interface Overview", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/ui/index.html"_s)}};
  root->slug = "ui";
  root->addChildren({converters, objedit, asmedit, symtab, cpudump, memdump, stkdump, iopane});
  return root;
}

QSharedPointer<HelpEntry> workflows_root() {
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  int mc10 = bitmask(PEP10, MC2);
  int oc10 = bitmask(PEP10, ISA3);
  int as10 = bitmask(PEP10, ASMB5);
  int p10 = mc10 | oc10 | as10;

  auto mc2 = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Microcode", "MDText.qml");
  mc2->props = QVariantMap{{"file", QVariant(u":/help/workflow/mc2.md"_s)}};
  mc2->isWIP = true;
  mc2->sortName = "2";

  auto isa3 = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Machine Language", "MDText.qml");
  isa3->props = QVariantMap{{"file", QVariant(u":/help/workflow/isa3.html"_s)}};
  isa3->sortName = "3a";

  auto asmb = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language", "MDText.qml");
  asmb->props = QVariantMap{{"file", QVariant(u":/help/workflow/asmb.html"_s)}};
  asmb->sortName = "5";

  auto auto_format =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Automatic Formatting", "MDText.qml");
  auto_format->props = QVariantMap{{"file", QVariant(u":/help/workflow/autoformat.html"_s)}};
  auto_format->sortName = "0a";

  auto breakpoints = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Breakpoints", "MDText.qml");
  breakpoints->props = QVariantMap{{"file", QVariant(u":/help/workflow/breakpoints.html"_s)}};
  breakpoints->sortName = "0b";

  auto step =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stepping through Programs", "MDText.qml");
  step->props = QVariantMap{{"file", QVariant(u":/help/workflow/step.html"_s)}};
  step->sortName = "0c";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Common Workflows", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/workflow/index.html"_s)}};
  root->slug = "flows";
  root->addChildren({auto_format, breakpoints, step, mc2, isa3, asmb});
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
  auto endless =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Handling Endless Loops", "MDText.qml");
  endless->props = QVariantMap{{"file", QVariant(u":/help/advanced/loops.md"_s)}};
  endless->isWIP = true;
  endless->sortName = "e";

  auto root = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Advanced Topics", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/advanced/_root.md"_s)}};
  root->isWIP = true;
  root->addChildren({brk_view, watch_expr, dbg_expr, brk_cond, endless});
  return root;
}

QSharedPointer<HelpEntry> greencard10_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "`C` bit on `SUBr`", "MDText.qml");
  c_bit->props = QVariantMap{{"file", QVariant(u":/help/pep9/c_bit.md"_s)}};
  c_bit->sortName = "001";
  auto n_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "`N` bit for `CPBr`", "MDText.qml");
  n_bit->props = QVariantMap{{"file", QVariant(u":/help/pep9/n_bit.md"_s)}};
  n_bit->sortName = "002";
  auto addr = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Addressing Modes", "MDText.qml");
  addr->props = QVariantMap{{"file", QVariant(u":/help/pep9/addr_modes.md"_s)}};
  addr->sortName = "003";
  auto reg = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Register field", "MDText.qml");
  reg->props = QVariantMap{{"file", QVariant(u":/help/pep9/register_fields.md"_s)}};
  reg->sortName = "004";
  auto mmio = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory-Mapped IO", "MDText.qml");
  mmio->props = QVariantMap{{"file", QVariant(u":/help/pep9/mmio.md"_s)}};
  mmio->sortName = "006";
  auto isa = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Instruction Set Architecture",
                                               "Greencard.qml");
  isa->props = QVariantMap{{"architecture", QVariant((int)pepp::Architecture::PEP10)}};
  isa->sortName = "0";
  isa->addChildren({c_bit, n_bit, addr, reg, mmio});

  auto asmb_macros = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Macros", "MDText.qml");
  asmb_macros->props = QVariantMap{{"file", QVariant(u":/help/pep10/asmb_using_macros.md"_s)}};
  asmb_macros->isWIP = true;
  asmb_macros->sortName = "0";
  auto asmb_symbols = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Symbols", "MDText.qml");
  asmb_symbols->props = QVariantMap{{"file", QVariant(u":/help/pep9/asmb_using_symbols.md"_s)}};
  asmb_symbols->isWIP = true;
  asmb_symbols->sortName = "1";
  auto asmb_scalls = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "System Calls", "MDText.qml");
  asmb_scalls->props = QVariantMap{{"file", QVariant(u":/help/pep10/asmb_using_scalls.md"_s)}};
  asmb_scalls->isWIP = true;
  asmb_scalls->sortName = "2";
  auto asmb_trace =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stack Trace Tags", "MDText.qml");
  asmb_trace->props = QVariantMap{{"file", QVariant(u":/help/pep9/asmb5_trace_tags.md"_s)}};
  asmb_trace->isWIP = true;
  asmb_trace->sortName = "3";
  auto asmb = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language", "MDText.qml");
  asmb->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  asmb->isWIP = true;
  asmb->sortName = "1";
  asmb->addChildren({asmb_macros, asmb_symbols, asmb_scalls, asmb_trace});

  auto scalls =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing System Calls", "MDText.qml");
  scalls->props = QVariantMap{{"file", QVariant(u":/help/pep10/os4_writing_scalls.md"_s)}};
  scalls->isWIP = true;
  scalls->sortName = "0";
  auto dot_section =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The `.SECTION` pseudo-op", "MDText.qml");
  dot_section->props = QVariantMap{{"file", QVariant(u":/help/pep10/os4_section.md"_s)}};
  dot_section->isWIP = true;
  dot_section->sortName = "1";
  auto dot_export =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The `.EXPORT` pseudo-op", "MDText.qml");
  dot_export->props = QVariantMap{{"file", QVariant(u":/help/pep10/os4_export.md"_s)}};
  dot_export->isWIP = true;
  dot_export->sortName = "2";
  auto dot_scall =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The `.SCALL` pseudo-op", "MDText.qml");
  dot_scall->props = QVariantMap{{"file", QVariant(u":/help/pep10/os4_scall.md"_s)}};
  dot_scall->isWIP = true;
  dot_scall->sortName = "3";
  auto dot_org =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The `.ORG` pseudo-op", "MDText.qml");
  dot_org->props = QVariantMap{{"file", QVariant(u":/help/pep10/os4_org.md"_s)}};
  dot_org->isWIP = true;
  dot_org->sortName = "4";
  auto os = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Operating System", "MDText.qml");
  os->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  os->isWIP = true;
  os->sortName = "2";
  os->addChildren({scalls, dot_section, dot_export, dot_scall, dot_org});

  auto alu = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "ALU Functions", "MDText.qml");
  alu->props = QVariantMap{{"file", QVariant(u":/help/pep9/alu_func.md"_s)}};
  alu->sortName = "0";
  auto mc = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Microcode", "MDText.qml");
  mc->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  mc->isWIP = true;
  mc->sortName = "3";
  mc->addChildren({alu});

  auto root =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, -1, "Pep/10 Reference", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  root->addChildren({isa, asmb, os, mc});
  return root;
}

QSharedPointer<HelpEntry> greencard9_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "`C` bit on `SUBr`", "MDText.qml");
  c_bit->props = QVariantMap{{"file", QVariant(u":/help/pep9/c_bit.md"_s)}};
  c_bit->sortName = "001";
  auto n_bit = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "`N` bit for `CPBr`", "MDText.qml");
  n_bit->props = QVariantMap{{"file", QVariant(u":/help/pep9/n_bit.md"_s)}};
  n_bit->sortName = "002";
  auto addr = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Addressing Modes", "MDText.qml");
  addr->props = QVariantMap{{"file", QVariant(u":/help/pep9/addr_modes.md"_s)}};
  addr->sortName = "003";
  auto reg = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Register field", "MDText.qml");
  reg->props = QVariantMap{{"file", QVariant(u":/help/pep9/register_fields.md"_s)}};
  reg->sortName = "004";
  auto mmio = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Memory-Mapped IO", "MDText.qml");
  mmio->props = QVariantMap{{"file", QVariant(u":/help/pep9/mmio.md"_s)}};
  mmio->sortName = "006";
  auto isa = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Instruction Set Architecture",
                                               "Greencard.qml");
  isa->props = QVariantMap{{"architecture", QVariant((int)pepp::Architecture::PEP9)}};
  isa->sortName = "0";
  isa->addChildren({c_bit, n_bit, addr, reg, mmio});

  auto asmb_symbols = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Symbols", "MDText.qml");
  asmb_symbols->props = QVariantMap{{"file", QVariant(u":/help/pep9/asmb_using_symbols.md"_s)}};
  asmb_symbols->isWIP = true;
  asmb_symbols->sortName = "1";
  auto asmb_trace =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Stack Trace Tags", "MDText.qml");
  asmb_trace->props = QVariantMap{{"file", QVariant(u":/help/pep9/asmb5_trace_tags.md"_s)}};
  asmb_trace->isWIP = true;
  asmb_trace->sortName = "2";
  auto asmb = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Assembly Language", "MDText.qml");
  asmb->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  asmb->isWIP = true;
  asmb->sortName = "0";
  asmb->addChildren({asmb_symbols, asmb_trace});

  auto traps =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Writing Trap Handlers", "MDText.qml");
  traps->props = QVariantMap{{"file", QVariant(u":/help/pep9/os4_writing_traps.md"_s)}};
  traps->isWIP = true;
  traps->sortName = "a";
  auto dot_burn =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "The `.BURN` pseudo-op", "MDText.qml");
  dot_burn->props = QVariantMap{{"file", QVariant(u":/help/pep9/os4_burn.md"_s)}};
  dot_burn->isWIP = true;
  dot_burn->sortName = "b";
  auto os = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Operating System", "MDText.qml");
  os->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  os->isWIP = true;
  os->sortName = "2";
  os->addChildren({traps, dot_burn});

  auto alu = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "ALU Functions", "MDText.qml");
  alu->props = QVariantMap{{"file", QVariant(u":/help/pep9/alu_func.md"_s)}};
  alu->sortName = "0";
  auto mc = QSharedPointer<HelpEntry>::create(HelpCategory::Category::Text, -1, "Microcode", "MDText.qml");
  mc->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  mc->isWIP = true;
  mc->sortName = "3";
  mc->addChildren({alu});

  auto root =
      QSharedPointer<HelpEntry>::create(HelpCategory::Category::ISAGreenCard, -1, "Pep/9 Reference", "MDText.qml");
  root->props = QVariantMap{{"file", QVariant(u":/help/blank.md"_s)}};
  root->addChildren({isa, asmb, os, mc});
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
