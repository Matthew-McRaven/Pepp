#include "architectures.hpp"
#include <map>
#include "core/ds/string_compare.hpp"
#include "spdlog/spdlog.h"

struct ArchData {
  std::string basic, pretty;
};

std::map<pepp::Architecture, ArchData> init_archs() {
  using namespace pepp;
  std::map<pepp::Architecture, ArchData> m{};
  m[Architecture::PEP8] = {"PEP8", "Pep/8"};
  m[Architecture::PEP9] = {"PEP9", "Pep/9"};
  m[Architecture::PEP10] = {"PEP10", "Pep/10"};
  m[Architecture::RISCV] = {"RISCV", "RISC-V"};
  return m;
}
static const auto archs = init_archs();

bool pepp::is_valid_arch(Architecture arch) noexcept { return archs.find(arch) != archs.end(); }

bool pepp::is_valid_arch(int arch) noexcept { return is_valid_arch(static_cast<Architecture>(arch)); }

std::string pepp::arch_as_string(Architecture architecture) {
  if (auto it = archs.find(architecture); it != archs.end()) return it->second.basic;
  SPDLOG_WARN("Unknown architecture in arch_as_string: {}", static_cast<int>(architecture));
  return "Unknown";
}

std::string pepp::arch_as_pretty_string(Architecture architecture) {
  if (auto it = archs.find(architecture); it != archs.end()) return it->second.pretty;
  SPDLOG_WARN("Unknown architecture in arch_as_pretty_string: {}", static_cast<int>(architecture));
  return "Unknown-Pretty";
}

pepp::Architecture pepp::string_to_arch(const std::string &str, bool *okay) {
  using namespace pepp::bts;
  for (const auto &it : archs) {
    if (ci_eq()(it.second.basic, str) || ci_eq()(it.second.pretty, str)) {
      if (okay) *okay = true;
      return it.first;
    }
  }
  if (okay) *okay = false;
  return Architecture::NO_ARCH;
}
