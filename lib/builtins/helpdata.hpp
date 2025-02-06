#pragma once
#include <QtCore>
#include <builtins/constants.hpp>

namespace builtins {
class Registry;
}
class HelpEntry;
QSharedPointer<HelpEntry> about_root();
QSharedPointer<HelpEntry> writing_root();
QSharedPointer<HelpEntry> debugging_root();
QSharedPointer<HelpEntry> systemcalls_root();
QSharedPointer<HelpEntry> greencard10_root();
QSharedPointer<HelpEntry> examples_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> problems_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> os_root();
QSharedPointer<HelpEntry> macros_root(const builtins::Registry &reg);
int bitmask(builtins::Architecture arch);
int bitmask(builtins::Abstraction level);
int bitmask(builtins::Architecture arch, builtins::Abstraction level);
bool masked(int lhs, int rhs);
