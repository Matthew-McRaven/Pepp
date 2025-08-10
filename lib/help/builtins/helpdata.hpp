#pragma once
#include <QtCore>
#include "enums/constants.hpp"

namespace builtins {
class Registry;
}
class HelpEntry;
QSharedPointer<HelpEntry> starting_root();
QSharedPointer<HelpEntry> ui_root();
QSharedPointer<HelpEntry> workflows_root();
QSharedPointer<HelpEntry> advanced_root();
QSharedPointer<HelpEntry> greencard10_root();
QSharedPointer<HelpEntry> greencard9_root();
QSharedPointer<HelpEntry> examples_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> problems_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> os_root();
QSharedPointer<HelpEntry> macros_root(const builtins::Registry &reg);
int bitmask(pepp::Architecture arch);
int bitmask(pepp::Abstraction level);
int bitmask(pepp::Architecture arch, pepp::Abstraction level);
bool masked(int lhs, int rhs);
