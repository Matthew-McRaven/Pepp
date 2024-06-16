#include "pepp.hpp"

#undef emit
#include "asm/parse/PeppLexer.h"
#include "asm/parse/PeppLexerErrorListener.h"
#include "asm/parse/PeppParser.h"
#include "asm/pas/parse/pepp/PeppASTConverter.h"

pas::driver::ParseResult pas::driver::pepp::detail::antlr4_pep10(const std::string &input,
                                                                 QSharedPointer<ast::Node> parent, bool hideEnd) {
  using namespace antlr4;
  using namespace parse;

  driver::ParseResult ret = {.hadError = false};
  ANTLRInputStream input_stream(input);
  ::parse::PeppLexer lexer(&input_stream);
  CommonTokenStream tokens(&lexer);
  ::parse::PeppLexerErrorListener listener{};
  // Don't write error to stdout by default.
  lexer.removeErrorListeners();
  lexer.addErrorListener(&listener);
  ::parse::PeppParser parser(&tokens);
  auto *tree = parser.prog();
  ::parse::PeppASTConverter converter(parent);
  auto ast = converter.visit(tree);
  ret.root = std::any_cast<QSharedPointer<pas::ast::Node>>(ast);
  if (listener.hadError()) {
    ret.hadError = true;
    ret.errors.push_back(u"Partial parse failure"_qs);
    return ret;
  }
  return ret;
}
