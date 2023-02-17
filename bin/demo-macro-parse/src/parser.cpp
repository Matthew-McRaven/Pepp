#include "parser.hpp"
#include "macro/parse.hpp"
MacroParseResult::MacroParseResult(QObject *parent) : QObject(parent) {}

MacroParseResult::MacroParseResult(QObject *parent, bool valid, QString name,
                                   quint8 argc)
    : QObject(parent), _valid(valid), _name(name), _argc(argc) {}

bool MacroParseResult::valid() const { return _valid; }

QString MacroParseResult::name() const { return _name; }

quint8 MacroParseResult::argc() const { return _argc; }

MacroParseResult *MacroParser::parse(QString arg) {
  // Parent should be nullptr, as we want to explicitly transfer ownership to
  // caller.
  auto parse = macro::analyze_macro_definition(arg);
  if (!std::get<0>(parse))
    return new MacroParseResult(nullptr);
  else
    return new MacroParseResult(nullptr, true, std::get<1>(parse),
                                std::get<2>(parse));
}
