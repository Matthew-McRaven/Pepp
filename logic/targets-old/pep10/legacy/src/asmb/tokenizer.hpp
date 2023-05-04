#pragma once
#include <fmt/format.h>

#include <boost/phoenix.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"
namespace asmb::pep10 {

using namespace boost::spirit::qi;
using namespace boost::spirit::ascii;
namespace lex = boost::spirit::lex;
static const std::string char_lit = {R"([^\n\\\"'])"};
static const std::string escape_lit = {R"(\\[{}bfnrtv\\])"};
static const std::string hex_lit = {R"(\\[xX][0-9a-fA-F]{{2}})"};
static const std::string inner_lit = "(" + char_lit + ")|(" + escape_lit + ")|(" + hex_lit + ")";
template <typename Lexer> struct tokenizer : lex::lexer<Lexer> {
    using token_type = masm::frontend::token_type;
    tokenizer()
        : macro_ident("@[A-Za-z_][A-Za-z0-9_]+"), macro_arg("\\$[0-9]+"), comma(","),
          identifier("[A-Za-z_][A-Za-z0-9|_]*"), sym_decl("[A-Za-z_][A-Za-z0-9|_]*:"), dot_command("\\.[a-zA-Z]+"),
          ch_lit(fmt::vformat("'({})'", fmt::make_format_args(fmt::vformat(inner_lit, fmt::make_format_args("'"))))),
          str_lit(fmt::vformat(R"(\"({})+\")",
                               fmt::make_format_args(fmt::vformat(inner_lit, fmt::make_format_args("\""))))),
          comment(";[^\n]*"), hex_literal("0[xX][0-9a-fA-F]+"),
          // Must escape +- or chaos ensues.
          dec_literal("[\\+\\-]?[0-9]+"), empty("\\n"), whitespaces("[( \\t)]+") {
        using boost::phoenix::construct;
        using boost::spirit::_1;
        std::cout << ch_lit;
        this->self += whitespaces[(lex::_pass = lex::pass_flags::pass_ignore)] |
                      macro_ident[(lex::_tokenid = (int)token_type::kMacroInvoke,
                                   lex::_val = construct<std::string>(lex::_start + 1, lex::_end))] |
                      sym_decl[(lex::_tokenid = (int)token_type::kSymbolDecl,
                                lex::_val = construct<std::string>(lex::_start, lex::_end - 1))] |
                      dot_command[(lex::_tokenid = (int)token_type::kDotDirective,
                                   lex::_val = construct<std::string>(lex::_start + 1, lex::_end))] |
                      macro_arg[(lex::_tokenid = (int)token_type::kMacroArg,
                                 lex::_val = construct<std::string>(lex::_start + 1, lex::_end))] |
                      comma[lex::_tokenid = (int)token_type::kComma] |
                      hex_literal[lex::_tokenid = (int)token_type::kHexConstant] |
                      dec_literal[lex::_tokenid = (int)token_type::kDecConstant] |
                      str_lit[(lex::_tokenid = (int)token_type::kStrConstant,
                               lex::_val = construct<std::string>(lex::_start + 1, lex::_end - 1))] |
                      ch_lit[(lex::_tokenid = (int)token_type::kCharConstant,
                              lex::_val = construct<std::string>(lex::_start + 1, lex::_end - 1))] |
                      identifier[lex::_tokenid = (int)token_type::kIdentifier] |
                      empty[(lex::_tokenid = (int)token_type::kEmpty, lex::_val = "")] |
                      comment[(lex::_tokenid = (int)token_type::kComment,
                               lex::_val = construct<std::string>(lex::_start + 1, lex::_end))];
    }

    lex::token_def<std::string> macro_ident, macro_arg, comma, identifier, sym_decl, dot_command, ch_lit, str_lit,
        empty, comment, hex_literal, dec_literal;
    lex::token_def<lex::omit> whitespaces;

    masm::frontend::token_type convert_token_to_type(masm::frontend::token_t token) {
        auto id = token.id();
        if (id == -1)
            return masm::frontend::token_type::kError;
        else
            return static_cast<masm::frontend::token_type>(id);
    }
};

}; // namespace asmb::pep10

#include "./tokenizer.tpp"