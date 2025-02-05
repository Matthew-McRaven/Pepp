#pragma once
#include <QtCore>
#include <builtins/constants.hpp>

class HelpEntry;
QSharedPointer<HelpEntry> about_root();
QSharedPointer<HelpEntry> writing_root();
QSharedPointer<HelpEntry> debugging_root();
QSharedPointer<HelpEntry> systemcalls_root();
QSharedPointer<HelpEntry> greencard10_root();
QSharedPointer<HelpEntry> examples_root(bool hotloaded = false);
QSharedPointer<HelpEntry> problems_root(bool hotloaded = false);
QSharedPointer<HelpEntry> os_root();
QSharedPointer<HelpEntry> macros_root(bool hotloaded = false);
int bitmask(builtins::Architecture arch);
int bitmask(builtins::Abstraction level);
int bitmask(builtins::Architecture arch, builtins::Abstraction level);
bool masked(int lhs, int rhs);
