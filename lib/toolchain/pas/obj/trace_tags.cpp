#include "trace_tags.hpp"
#include "toolchain/pas/operations/generic/trace_tags.hpp"

static const auto traceStr = ".debug_trace";

void pas::obj::common::writeDebugCommands(ELFIO::elfio &elf, ast::Node &root) {
  auto trace = detail::getOrAddTraceSection(elf);
  auto tt = ops::generic::extractTraceTags(root);
}

ELFIO::section *pas::obj::common::detail::getTraceSection(ELFIO::elfio &elf) {
  ELFIO::section *trace = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == traceStr && sec->get_type() == ELFIO::SHT_PROGBITS) {
      trace = sec.get();
      break;
    }
  }
  return trace;
}

ELFIO::section *pas::obj::common::detail::getOrAddTraceSection(ELFIO::elfio &elf) {
  auto trace = getTraceSection(elf);
  if (trace == nullptr) {
    trace = elf.sections.add(traceStr);
    trace->set_type(ELFIO::SHT_PROGBITS);
  }
  return trace;
}
