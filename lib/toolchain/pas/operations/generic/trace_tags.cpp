#include "trace_tags.hpp"
#include <iterator>
#include "pep10.hpp"
#include "pep9.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_comment.hpp"
#include "toolchain/pas/ast/generic/attr_comment_indent.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/pepp/attr_instruction.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"

QString format(const QString &cmd, const QStringList &args) {
  using namespace Qt::StringLiterals;
  auto trimmed = args.join("#").replace("\\W*", "");
  if (cmd.isEmpty()) return u"#%1"_s.arg(trimmed);
  else if (args.empty()) return u"@%1"_s.arg(cmd.trimmed());
  return u"@%1#%2"_s.arg(cmd.trimmed(), trimmed);
}

pas::ops::generic::TraceMatch::operator QString() const { return format(command, args); }

std::optional<std::vector<pas::ops::generic::TraceMatch>> pas::ops::generic::parseTraceCommand(const QString &comment) {
  static const QRegularExpression re("[#@]\\w+");
  static const QRegularExpression space("\\W+");

  auto match = re.globalMatch(comment);
  if (!match.hasNext()) return std::nullopt;

  std::vector<pas::ops::generic::TraceMatch> ret;
  QString cmd;
  QStringList args;

  for (auto it = match; it.hasNext();) {
    auto m = it.next();
    if (m.capturedLength() == 0) continue;
    auto text = m.capturedView();

    if (text.startsWith('@')) {
      // If the command is not empty, we should emit it before overwriting it.
      if (!cmd.isEmpty()) {
        ret.emplace_back(TraceMatch{cmd, args});
        args.clear();
      }
      cmd = text.toString().replace('@', "");
    } else if (text.startsWith('#')) args.emplaceBack(text.toString().replace('#', ""));
  }
  if (!cmd.isEmpty() || !args.isEmpty()) ret.emplace_back(TraceMatch{cmd, args});

  return ret;
}

bool pas::ops::generic::isTypeTag(const QStringView &str) {
  static const QRegularExpression re("\\d+[cdsuh](\\d+a)?");
  if (re.matchView(str).hasMatch()) return true;
  return false;
}

QString pas::ops::generic::infer_command(const ast::Node &node, const QStringList &args) {
  static const auto equate_alias = QList<QString>{"EQUATE"};
  static const auto global_alias = QList<QString>{"BLOCK", "BYTE", "WORD", "ADDRSS"};

  auto equate = pas::ops::generic::isSet{};
  equate.directiveAliases = equate_alias;
  auto global = pas::ops::generic::isSet{};
  global.directiveAliases = global_alias;
  if (equate(node)) {
    bool allTypes = true;
    for (const auto &arg : args) allTypes &= isTypeTag(arg);
    if (allTypes) return "type";
    return "struct";
  } else if (global(node)) {
    bool allTypes = true;
    for (const auto &arg : args) allTypes &= isTypeTag(arg);
    if (allTypes) return "global";
    return "struct";
  } else if (pepp::isNonUnary<isa::Pep10>()(node)) {
    auto instr = node.get<pas::ast::pepp::Instruction<isa::Pep10>>();
    switch (instr.value) {
    case isa::detail::pep10::Mnemonic::CALL: return "heap";
    default: return "params";
    }
  } else if (pepp::isNonUnary<isa::Pep9>()(node)) {
    auto instr = node.get<ast::pepp::Instruction<isa::Pep9>>();
    switch (instr.value) {
    case isa::detail::pep9::Mnemonic::CALL: return "heap";
    default: return "params";
    }
  } else if (node.has<ast::generic::SymbolDeclaration>()) return "locals";
  else return "params";
}

void pas::ops::generic::ExtractTraceTags::operator()(ast::Node &node) {
  using namespace pas::ast::generic;
  std::optional<std::vector<pas::ops::generic::TraceMatch>> match = std::nullopt;

  if (node.has<IsMacroComment>()) return;
  else if (node.has<Comment>()) match = pas::ops::generic::parseTraceCommand(node.get<Comment>().value);
  else return;

  if (match) {
    wip_commands.insert(wip_commands.end(), std::make_move_iterator(match->begin()),
                        std::make_move_iterator(match->end()));
  } else return;

  std::optional<uint32_t> address = std::nullopt;
  if (node.has<Address>()) address = node.get<Address>().value.start;

  // If line is not a comment, then we assign all queued commands to this line, clearing the queue.
  if (!isComment()(node)) {
    QString cmd_str = "";
    for (const auto &cmd : wip_commands) {
      cmd_str = cmd.command;
      if (cmd_str.isEmpty()) cmd_str = infer_command(node, cmd.args);
      commands.push_back(Command{cmd_str, cmd.args, address});
    }
    wip_commands.clear();
  }
}

std::vector<pas::ops::generic::Command> pas::ops::generic::extractTraceTags(ast::Node &node) {
  ExtractTraceTags visitor;
  pas::ast::apply_recurse(node, visitor);
  for (const auto &cmd : visitor.commands) qDebug().noquote() << (QString)cmd;
  return visitor.commands;
}

pas::ops::generic::Command::operator QString() const {
  using namespace Qt::StringLiterals;
  if (address) return u"%1: %2"_s.arg(*address, 4, 16).arg(format(command, args));
  else return u"      %1"_s.arg(format(command, args));
}
