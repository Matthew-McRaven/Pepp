#pragma once
#include "pas/ast/node.hpp"
#include <QtCore>
#include <elfio/elfio.hpp>

namespace pas::obj::pep10 {
void combineSections(pas::ast::Node &root);
ELFIO::elfio createElf();
void writeOS(ELFIO::elfio &elf, pas::ast::Node &os);
void writeUser(ELFIO::elfio &elf, pas::ast::Node &user);
void writeUser(ELFIO::elfio &elf, QList<quint8> bytes);
} // namespace pas::obj::pep10
