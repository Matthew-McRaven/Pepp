#include "pepp.hpp"

#undef emit
#include "toolchain/parse/PeppLexer.h"
#include "toolchain/parse/PeppLexerErrorListener.h"
#include "toolchain/parse/PeppParser.h"
#include "toolchain/pas/parse/pepp/PeppASTConverter10.h"
#include "toolchain/pas/parse/pepp/PeppASTConverter9.h"

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
    ret.errors.push_back("Partial parse failure");
    return ret;
  }
  return ret;
}

pas::driver::ParseResult pas::driver::pepp::detail::antlr4_pep9(const std::string &input,
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
  ::parse::PeppASTConverter9 converter(parent);
  auto ast = converter.visit(tree);
  ret.root = std::any_cast<QSharedPointer<pas::ast::Node>>(ast);
  if (listener.hadError()) {
    ret.hadError = true;
    ret.errors.push_back("Partial parse failure");
    return ret;
  }
  return ret;
}
