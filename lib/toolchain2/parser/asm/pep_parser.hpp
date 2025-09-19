#pragma once
#include <memory>
#include "toolchain/symbol/table.hpp"
#include "toolchain2/ir/asm/pep_common.hpp"
#include "toolchain2/lex/asm/pep_lexer.hpp"
#include "toolchain2/lex/core/buffer.hpp"
#include "toolchain2/support/allocators/string_pool.hpp"

namespace pepp::tc::parser {
struct PepParser {
  PepParser(support::SeekableData &&data);

  std::vector<std::shared_ptr<tc::ir::LinearIR>> parse();
  std::shared_ptr<symbol::Table> symbol_table() const;

private:
  std::shared_ptr<pas::ast::value::Base> argument();
  std::shared_ptr<ir::LinearIR> instruction();
  std::shared_ptr<ir::LinearIR> dot_command();

  std::shared_ptr<support::StringPool> _pool;
  std::shared_ptr<lex::PepLexer> _lexer;
  std::shared_ptr<lex::Buffer> _buffer;
  std::shared_ptr<symbol::Table> _symtab;
};
} // namespace pepp::tc::parser
