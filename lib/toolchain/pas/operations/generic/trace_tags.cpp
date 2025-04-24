#include "trace_tags.hpp"

pas::ops::generic::TraceMatch::operator QString() const { return command + "" + args.join(""); }

std::optional<pas::ops::generic::TraceMatch> pas::ops::generic::parseTraceCommand(const QString &comment) {
  return std::nullopt;
}

void pas::ops::generic::ExtractTraceTags::operator()(ast::Node &node) {}

std::map<quint32, QString> pas::ops::generic::extractTraceTags(ast::Node &node) {
  ExtractTraceTags visitor;
  node.apply_self(visitor);
  return visitor.commands;
}
