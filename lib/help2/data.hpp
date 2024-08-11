#pragma once
#include "entry.hpp"

static const QSharedPointer<HelpEntry> about_root() {
  return QSharedPointer<HelpEntry>::create(HelpEntry{
      .category = HelpEntry::Category::About,
      .tags = 0,
      .name = "About",
      .delgate = "about",
  });
};
static const QSharedPointer<HelpEntry> writing_root() {
  auto microcode10_language = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Microcode",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/writing_mc.md"_qs)}}});
  auto machine10_language = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Machine Language",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/writing_oc.md"_qs)}}});
  auto assembly10_language = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Assembly Language",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/writing_asmb.md"_qs)}}});
  return QSharedPointer<HelpEntry>::create(HelpEntry{
      .category = HelpEntry::Category::Text,
      .tags = 0,
      .name = "Writing Programs",
      .delgate = "text",
      .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/writing_progs.md"_qs)}},
      .children = {microcode10_language, machine10_language, assembly10_language},
  });
};

static const QSharedPointer<HelpEntry> debugging_root() {
  return QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Debugging Programs",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/debugging_progs.md"_qs)}}});
};

static const QSharedPointer<HelpEntry> systemcalls_root() {
  return QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Writing System Calls",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/writing_scalls.md"_qs)}}});
};

static const QSharedPointer<HelpEntry> greencard10_root() {
  auto c_bit = QSharedPointer<HelpEntry>::create(HelpEntry{
      .category = HelpEntry::Category::Text,
      .tags = 0,
      .name = "Setting the C bit on subtraction",
      .delgate = "text",
      .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/c_bit.md"_qs)}},
      .showInParent = true,
  });
  auto n_bit = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Setting the N bit on <mono>CPr</mono>",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/n_bit.md"_qs)}},
                .showInParent = true});
  auto addr = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Addressing Modes",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/addr_modes.md"_qs)}},
                .showInParent = true});
  auto reg = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Register field",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/register_fields.md"_qs)}},
                .showInParent = true});
  auto mmio = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "Memory-mapped IO",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/mmio.md"_qs)}},
                .showInParent = true});
  auto alu = QSharedPointer<HelpEntry>::create(
      HelpEntry{.category = HelpEntry::Category::Text,
                .tags = 0,
                .name = "ALU Functions",
                .delgate = "text",
                .props = QVariantMap{{"file", QVariant(u"qrc:/ui/help/pep10/alu_func.md"_qs)}},
                .showInParent = true});
  return QSharedPointer<HelpEntry>::create(HelpEntry{.category = HelpEntry::Category::ISAGreenCard,
                                                     .tags = 0,
                                                     .name = "Pep/10 Reference",
                                                     .delgate = "ISA",
                                                     .children = {c_bit, n_bit, addr, reg, mmio, alu}});
};

static const QSharedPointer<HelpEntry> examples_root() { return {}; }
static const QSharedPointer<HelpEntry> problems_root() { return {}; }
static const QSharedPointer<HelpEntry> os_root() {
  return QSharedPointer<HelpEntry>::create(HelpEntry{
      .category = HelpEntry::Category::Figure,
      .tags = 0,
      .name = "Pep/10 Operating System",
      .delgate = "figure",
  });
}
