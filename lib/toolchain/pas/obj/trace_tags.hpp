#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
#include "toolchain/pas/ast/generic/attr_sec.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/operations/generic/combine.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "toolchain/symbol/entry.hpp"

namespace pas::obj::common {
void writeDebugCommands(ELFIO::elfio &elf, pas::ast::Node &root);
namespace detail {
// Get line mapping section or return nullptr;
ELFIO::section *getTraceSection(ELFIO::elfio &elf);
ELFIO::section *getOrAddTraceSection(ELFIO::elfio &elf);
} // namespace detail
} // namespace pas::obj::common
