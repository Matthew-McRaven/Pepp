#pragma once

#include <QDebug>
#include <boost/spirit/home/x3.hpp>
class parser {
public:
  parser();
};

template <typename Iterator> bool parse_numbers(Iterator first, Iterator last) {
  using boost::spirit::x3::double_;
  using boost::spirit::x3::phrase_parse;
  using boost::spirit::x3::ascii::space;

  bool r = phrase_parse(first,                        //  Start Iterator
                        last,                         //  End Iterator
                        double_ >> *(',' >> double_), //  The Parser
                        space                         //  The Skip-Parser
  );
  if (first != last) // fail if we did not get a full match
    return false;
  return r;
}

struct print_action {
  template <typename Context> void operator()(Context const &ctx) const {
    auto x = _attr(ctx);
    auto string = QString::fromStdString(x);
    qWarning() << string;
  }
};

template <typename Iterator> bool parse_macro(Iterator first, Iterator last) {
  using boost::spirit::x3::double_;
  using boost::spirit::x3::lexeme;
  using boost::spirit::x3::phrase_parse;
  using boost::spirit::x3::ascii::char_;
  using boost::spirit::x3::ascii::space;

  bool r = phrase_parse(first, last,
                        lexeme["@" >> +(char_ - space)][print_action()], space);
  if (first != last) // fail if we did not get a full match
    return false;
  return r;
}
