#include "parse.hpp"
#include <regex>
#include <string>
#include <string_view>
#include <tuple>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/object.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/spirit/include/qi.hpp>

#include <iostream>

namespace detail {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct macro {
  std::string name = {};
  int arg_count = {0};
};
template <typename Iterator>
struct macro_parser : qi::grammar<Iterator, macro(), ascii::space_type> {
  macro_parser() : macro_parser::base_type(start) {
    using ascii::char_;
    using qi::double_;
    using qi::int_;
    using qi::lexeme;
    using qi::lit;
    using qi::no_skip;

    start %=
        qi::no_skip['@'] >> qi::lexeme[+(qi::char_ - ascii::space)] >> int_;
  }
  qi::rule<Iterator, macro(), ascii::space_type> start;
};
}; // namespace detail

BOOST_FUSION_ADAPT_STRUCT(::detail::macro, (std::string, name)(int, arg_count))

std::tuple<bool, QString, quint8>
macro::analyze_macro_definition(QString macro_text) {
  /*
   * A macro file must begin with with name of the macro, followed by an
   * arbitrary number of spaces followed by an integer in [0,16] specifying the
   * number of arguments the macro takes.
   *
   * Neither comments nor whitespace may occur before this definition line.
   *
   * Below are valid examples:
   * @DECI 2
   * @UNOP 0
   *
   * Below are invalid examples, with comments descrbing why.
   *
   * Whitepace preceding macro definiton:
   *      @DECI 2
   * Space between macro name and macro symbol @.
   * @ deci 2
   *
   * Line ends in a comment
   * @deci 2 ;My comment
   *
   */
  auto as_std = macro_text.toUtf8().toStdString();
  std::string first_line = as_std.substr(0, as_std.find("\n"));
  detail::macro macro;
  using boost::spirit::ascii::space;
  using iterator_type = std::string::const_iterator;
  using macro_parser = detail::macro_parser<iterator_type>;
  macro_parser g;
  auto cbegin = first_line.cbegin();
  bool r = boost::spirit::qi::phrase_parse(cbegin, first_line.cend(), g,
                                           boost::spirit::ascii::space, macro);
  // If we failed, or if we did not consume the entire input, fail.
  if (!r || cbegin != first_line.cend()) {
    return {false, "", 0};
  }
  return {true, QString::fromStdString(macro.name), macro.arg_count};
}
