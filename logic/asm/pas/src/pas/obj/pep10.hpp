#pragma once
#include "pas/ast/node.hpp"
#include <QtCore>
#include <elfio/elfio.hpp>
#include "pas/pas_globals.hpp"

namespace pas::obj::pep10 {
void PAS_EXPORT combineSections(pas::ast::Node &root);
QSharedPointer<ELFIO::elfio> PAS_EXPORT createElf();
void PAS_EXPORT writeOS(ELFIO::elfio &elf, pas::ast::Node &os);
void PAS_EXPORT writeUser(ELFIO::elfio &elf, pas::ast::Node &user);
void PAS_EXPORT writeUser(ELFIO::elfio &elf, QList<quint8> bytes);
} // namespace pas::obj::pep10
