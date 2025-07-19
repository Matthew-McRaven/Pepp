#include "trace_tags.hpp"
#include "toolchain/pas/operations/generic/trace_tags.hpp"
#include "zpp_bits.h"

static const auto traceStr = ".debug_trace";

void pas::obj::common::writeDebugCommands(ELFIO::elfio &elf, std::list<ast::Node *> roots) {
  auto trace = detail::getOrAddTraceSection(elf);
  auto [data, in, out] = zpp::bits::data_in_out();
  for (const auto &root : roots) {
    auto tt = ops::generic::extractTraceTags(*root);
    (void)out(tt);
  }

  trace->append_data((const char *)data.data(), data.size());
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
