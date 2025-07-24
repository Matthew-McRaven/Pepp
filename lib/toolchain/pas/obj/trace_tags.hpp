#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
#include "sim/debug/expr_rtti.hpp"
#include "sim/debug/stack_ops.hpp"
#include "toolchain/pas/ast/generic/attr_sec.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/operations/generic/combine.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "toolchain/symbol/entry.hpp"

namespace pas::obj::common {
void writeDebugCommands(ELFIO::elfio &elf, std::list<pas::ast::Node *> roots);
struct DebugInfo {
  std::shared_ptr<pepp::debug::types::TypeInfo> typeInfo; // Type information for the debug session.
  QMap<quint32, pepp::debug::CommandFrame> commands;      // Maps address to command packets.
};

DebugInfo readDebugCommands(ELFIO::elfio &elf);
namespace detail {
// Get line mapping section or return nullptr;
ELFIO::section *getTraceSection(ELFIO::elfio &elf);
ELFIO::section *getOrAddTraceSection(ELFIO::elfio &elf);
} // namespace detail
} // namespace pas::obj::common
